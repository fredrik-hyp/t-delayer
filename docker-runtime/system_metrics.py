"""Small Linux system sampler; no subprocesses or directory scans per refresh."""
import os
from pathlib import Path
import threading
import time


class SystemMetrics:
    def __init__(self, proc_root="/proc"):
        self.proc_root = Path(proc_root)
        self.lock = threading.Lock()
        self.previous_cpu = None
        self.previous_network = None
        self.sample_time = None
        self.cached = None

    def read_cpu(self):
        with (self.proc_root / "stat").open() as stream:
            fields = stream.readline().split()
        if fields[0] != "cpu" or len(fields) < 5:
            raise ValueError("Missing aggregate CPU counters")
        # guest and guest_nice are already included in user and nice.
        ticks = [int(value) for value in fields[1:9]]
        return sum(ticks), ticks[3] + (ticks[4] if len(ticks) > 4 else 0)

    def read_network(self):
        counters = {}
        for line in (self.proc_root / "net/dev").read_text().splitlines()[2:]:
            interface, values = line.split(":", 1)
            interface = interface.strip()
            if interface != "lo":
                fields = values.split()
                counters[interface] = (int(fields[0]), int(fields[8]))
        return counters

    def sample(self, storage_path):
        # Status requests from several browsers share the same sampling window.
        with self.lock:
            now = time.monotonic()
            if self.cached is not None and now - self.sample_time < 1:
                return self.cached
            elapsed = now - self.sample_time if self.sample_time is not None else 0
            result = {"storage": None, "cpu": None, "network": None}
            try:
                disk = os.statvfs(storage_path or "")
                total = disk.f_blocks * disk.f_frsize
                available = disk.f_bavail * disk.f_frsize
                result["storage"] = {"path": str(storage_path), "total_bytes": total,
                                     "available_bytes": available,
                                     "used_percent": 100 * (total - available) / total if total else None}
            except (OSError, ValueError):
                pass
            try:
                cpu = self.read_cpu()
                if self.previous_cpu is not None:
                    total = cpu[0] - self.previous_cpu[0]
                    idle = cpu[1] - self.previous_cpu[1]
                    if total > 0 and 0 <= idle <= total:
                        result["cpu"] = {"usage_percent": 100 * (total - idle) / total}
                self.previous_cpu = cpu
            except (OSError, ValueError, IndexError):
                self.previous_cpu = None
            try:
                network = self.read_network()
                if self.previous_network is not None and elapsed > 0:
                    # Compare each interface independently: new/disappearing NICs
                    # and counter resets must not produce negative rates or spikes.
                    deltas = [(rx - self.previous_network[name][0], tx - self.previous_network[name][1])
                              for name, (rx, tx) in network.items() if name in self.previous_network
                              and rx >= self.previous_network[name][0] and tx >= self.previous_network[name][1]]
                    if deltas or not network:
                        result["network"] = {"rx_bps": sum(rx for rx, _ in deltas) * 8 / elapsed,
                                             "tx_bps": sum(tx for _, tx in deltas) * 8 / elapsed,
                                             "interfaces": sorted(network)}
                self.previous_network = network
            except (OSError, ValueError, IndexError):
                self.previous_network = None
            self.sample_time = now
            self.cached = result
            return result
