from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from system_metrics import SystemMetrics


class SystemMetricsTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / 'net').mkdir()
        self.metrics = SystemMetrics(self.root)
        self.now = 100
        clock = patch('system_metrics.time.monotonic', side_effect=lambda: self.now)
        clock.start()
        self.addCleanup(clock.stop)
        self.write_cpu(100, 100)
        self.write_network(1000, 2000)

    def write_cpu(self, active, idle, guest=0):
        (self.root / 'stat').write_text(f'cpu {active} 0 0 {idle} 0 0 0 0 {guest} 0\ncpu0 0 0 0 0\n')

    def write_network(self, rx, tx, extra=''):
        (self.root / 'net/dev').write_text('header\nheader\n'
            f'lo: {self.now * 10000} 0 0 0 0 0 0 0 {self.now * 10000} 0 0 0 0 0 0 0\n'
            f'eth0: {rx} 0 0 0 0 0 0 0 {tx} 0 0 0 0 0 0 0\n' + extra)

    def test_rates_use_elapsed_time_and_exclude_loopback_and_guest_double_counting(self):
        first = self.metrics.sample(self.root)
        self.assertIsNone(first['cpu'])
        self.assertIsNone(first['network'])
        self.now += 2
        self.write_cpu(150, 150, guest=30)
        self.write_network(251000, 502000)
        sample = self.metrics.sample(self.root)
        self.assertEqual(sample['cpu']['usage_percent'], 50)
        self.assertEqual(sample['network'], {'rx_bps': 1000000, 'tx_bps': 2000000, 'interfaces': ['eth0']})

    def test_disk_uses_recording_path_and_available_blocks_not_reserved_free_blocks(self):
        disk = SimpleNamespace(f_blocks=100, f_bfree=40, f_bavail=30, f_frsize=4096)
        with patch('system_metrics.os.statvfs', return_value=disk) as statvfs:
            storage = self.metrics.sample('/recording')['storage']
        statvfs.assert_called_once_with('/recording')
        self.assertEqual(storage['available_bytes'], 30 * 4096)
        self.assertEqual(storage['total_bytes'], 100 * 4096)
        self.assertEqual(storage['used_percent'], 70)

    def test_clients_share_sampling_window(self):
        first = self.metrics.sample(self.root)
        self.now += .2
        with patch.object(self.metrics, 'read_cpu', side_effect=AssertionError('resampled')):
            self.assertIs(self.metrics.sample(self.root), first)

    def test_missing_metrics_do_not_break_storage_and_recover_without_stale_rates(self):
        self.metrics.sample(self.root)
        (self.root / 'stat').unlink()
        (self.root / 'net/dev').write_text('header\nheader\ninvalid')
        self.now += 2
        sample = self.metrics.sample(self.root)
        self.assertIsNotNone(sample['storage'])
        self.assertIsNone(sample['cpu'])
        self.assertIsNone(sample['network'])
        self.write_cpu(150, 150)
        self.write_network(3000, 4000)
        self.now += 2
        sample = self.metrics.sample(None)
        self.assertIsNone(sample['storage'])
        self.assertIsNone(sample['cpu'])
        self.assertIsNone(sample['network'])
        self.now += 2
        self.write_cpu(175, 225)
        self.write_network(5000, 8000)
        sample = self.metrics.sample(self.root)
        self.assertEqual(sample['cpu']['usage_percent'], 25)
        self.assertEqual(sample['network']['rx_bps'], 8000)

    def test_reset_counters_and_new_interfaces_do_not_cause_rate_spikes(self):
        self.metrics.sample(self.root)
        self.now += 2
        self.write_cpu(1, 1)
        self.write_network(0, 0, 'eth1: 9999999999 0 0 0 0 0 0 0 9999999999 0 0 0 0 0 0 0\n')
        sample = self.metrics.sample(self.root)
        self.assertIsNone(sample['cpu'])
        self.assertIsNone(sample['network'])
        self.now += 2
        self.write_cpu(101, 101)
        self.write_network(2000, 4000)
        sample = self.metrics.sample(self.root)
        self.assertEqual(sample['network']['rx_bps'], 8000)
        self.assertEqual(sample['network']['tx_bps'], 16000)


if __name__ == '__main__':
    unittest.main()
