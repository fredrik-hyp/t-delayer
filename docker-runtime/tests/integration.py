"""Run inside the test runtime container: actual nginx, DB, C workers and UDP."""
import base64
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import re
import signal
import socket
import struct
import subprocess
import threading
import time
import urllib.error
import urllib.request

BASE = os.environ.get('TEST_BASEURL', 'http://127.0.0.1:8080')
AUTH = 'Basic ' + base64.b64encode(('admin:' + os.environ['WEB_PASSWORD']).encode()).decode()
produced_times = {}


def request(path, body=None, auth=True, control=True):
    headers = {'Authorization': AUTH} if auth else {}
    if body is not None:
        headers['Content-Type'] = 'application/json'
        if control:
            headers['X-Delayer-Control'] = '1'
    req = urllib.request.Request(BASE+path, data=json.dumps(body).encode() if body is not None else None, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=35) as response:
            data = response.read()
            return response.status, json.loads(data) if response.headers.get_content_type() == 'application/json' else data
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def status(name='st1'):
    code, data = request('/api/status')
    assert code == 200, (code,data)
    return next(item for item in data['instances'] if item['name'] == name)


def wait_for(predicate, description, timeout=20):
    deadline = time.monotonic()+timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(.2)
    raise AssertionError('Timeout: '+description+'; status='+json.dumps(request('/api/status')[1]))


def command(path, action, name='st1', **fields):
    code, data = request(f'/api/instances/{name}/outputs/{path}/commands', dict(action=action, **fields))
    assert code == 202, (code,data)
    return data


class Receiver:
    def __init__(self, group):
        self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.sock.bind((group,1234))
        self.sock.setsockopt(socket.IPPROTO_IP,socket.IP_ADD_MEMBERSHIP,socket.inet_aton(group)+socket.inet_aton('0.0.0.0'))
        self.sock.settimeout(.2)
        self.packets = []
        self.done = False
        self.thread = threading.Thread(target=self.run,daemon=True)
        self.thread.start()
    def run(self):
        while not self.done:
            try:
                payload = self.sock.recv(2048)
                self.packets.append((time.monotonic(),payload))
            except socket.timeout:
                pass
    def count(self):
        return len(self.packets)


def producer(group, stop):
    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    sock.setsockopt(socket.IPPROTO_IP,socket.IP_MULTICAST_TTL,1)
    sock.setsockopt(socket.IPPROTO_IP,socket.IP_MULTICAST_LOOP,1)
    index = 0
    produced_times[group] = {}
    while not stop.is_set():
        # 7 TS packets in a minimal RTP packet. Sequence is visible in captures.
        ts = b''.join(b'\x47\x1f\xff\x10'+struct.pack('!I',index)+bytes(180) for _ in range(7))
        rtp = struct.pack('!BBHII',0x80,33,index%65536,index*1800,123)
        produced_times[group][index] = time.monotonic()
        sock.sendto(rtp+ts,(group,1234))
        index += 1
        stop.wait(.02)
    sock.close()


stop = threading.Event()
receivers = [Receiver(group) for group in ['239.20.0.1','239.20.0.2','239.20.0.3']]
for group in ['239.10.0.1','239.10.0.2']:
    threading.Thread(target=producer,args=(group,stop),daemon=True).start()
try:
    assert request('/api/status',auth=False)[0] == 401
    assert request('/')[0] == 200
    assert request('/api/instances/st1/outputs/0/commands',{'action':'pause'},control=False)[0] == 403
    assert request('/api/instances/not-managed/start',{})[0] == 404
    wait_for(lambda: status()['process_state']=='running' and status('st2')['process_state']=='running','two ready instances')
    initial = {name:status(name)['pid'] for name in ['st1','st2']}
    wait_for(lambda: all(receiver.count()>30 for receiver in receivers),'live traffic on all three outputs')
    assert len(receivers[0].packets[-1][1]) == 1328
    assert len(receivers[1].packets[-1][1]) == 1316
    assert receivers[1].packets[-1][1][0] == 0x47
    print('PASS nginx authentication, CSRF guard, two processes, 3 outputs, RTP/raw UDP',flush=True)

    # Let two chunks finish before exercising on-disk seeks.
    time.sleep(12)
    command(0,'pause')
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Pause','output 0 pause')
    time.sleep(.5)
    counts = [receiver.count() for receiver in receivers]
    time.sleep(1)
    assert receivers[0].count() == counts[0]
    assert receivers[1].count() > counts[1]+20 and receivers[2].count() > counts[2]+20
    print('PASS pause output 0 leaves output 1 and other instance transmitting',flush=True)

    command(0,'play')
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Play','delayed playback')
    wait_for(lambda: receivers[0].count()>counts[0]+160,'delayed traffic resumes')
    settled_count = receivers[0].count()
    wait_for(lambda: receivers[0].count()>settled_count+300,'steady replay window')
    samples = receivers[0].packets[-250:]
    span = samples[-1][0]-samples[0][0]
    first_sequence = struct.unpack('!H', samples[0][1][2:4])[0]
    last_sequence = struct.unpack('!H', samples[-1][1][2:4])[0]
    recorded_span = produced_times['239.10.0.1'][last_sequence] - produced_times['239.10.0.1'][first_sequence]
    # Allow VM scheduling/batching at the ends while rejecting unpaced playback.
    assert recorded_span > 3 and abs(span-recorded_span) < 1.25, (span,recorded_span)
    print('PASS resumed replay follows recorded packet timing',flush=True)

    # API rejects malformed/out-of-buffer times before touching the worker.
    assert request('/api/instances/st1/outputs/99/commands',{'action':'pause'})[0] == 404
    assert request('/api/instances/st1/outputs/0/commands',{'action':'jump','jumptopos':'2000-01-01T00:00:00Z'})[0] == 409
    assert request('/api/instances/st1/outputs/0/commands',{'action':'jump','jumptopos':'2026-01-01T00:00:00'})[0] == 400

    now = datetime.now(timezone.utc)
    recorded = (now-timedelta(seconds=8)).isoformat()
    starts = (now+timedelta(seconds=4)).isoformat()
    ends = (now+timedelta(seconds=8)).isoformat()
    command(0,'jump',jumptopos=recorded,startplayat=starts,playto=ends)
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Pause','scheduled jump pauses first')
    scheduled = status()
    assert scheduled['outputs'][0]['status']['startplayat']
    assert scheduled['outputs'][1]['status']['startplayat'] is None
    assert scheduled['outputs'][1]['status']['playto'] is None
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Play','scheduled playback starts')
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Pause','wall clock playto stops output')
    assert status()['outputs'][1]['status']['state']=='Live'
    print('PASS jump, scheduled start, wall-clock stop, independent schedule status',flush=True)

    # Burst commands across outputs: coalesced SIGUSR1 must not lose queue entries.
    for path, action in [(0,'play'),(1,'pause'),(0,'pause'),(1,'play')]:
        command(path,action)
    wait_for(lambda: not status()['pending'],'burst command queue drains')
    wait_for(lambda: status()['outputs'][0]['status']['state']=='Pause' and status()['outputs'][1]['status']['state']=='Play','burst final states')
    for name,pid in initial.items():
        assert status(name)['pid']==pid, 'control restarted a process'
    print('PASS burst commands drain, output state isolated, PIDs unchanged',flush=True)

    # Independent process stop/start must leave the other instance alive.
    assert request('/api/instances/st1/stop',{})[0] == 200
    assert status()['process_state']=='stopped'
    assert status('st2')['pid']==initial['st2']
    assert request('/api/instances/st1/outputs/0/commands',{'action':'play'})[0] == 409
    assert request('/api/instances/st1/start',{})[0] == 202
    wait_for(lambda: status()['process_state']=='running','manual instance restart')
    assert status()['pid'] != initial['st1']
    print('PASS independent process stop/start and stopped-output rejection',flush=True)

    # Stale metrics must not permit sending signals; stopped children are detected.
    frozen_pid = status('st2')['pid']
    os.kill(frozen_pid, signal.SIGSTOP)
    try:
        wait_for(lambda: status('st2')['outputs'][0]['stale'], 'stale statistics detected', timeout=15)
        assert request('/api/instances/st2/outputs/0/commands', {'action':'pause'})[0] == 409
    finally:
        os.kill(frozen_pid, signal.SIGCONT)
    wait_for(lambda: status('st2')['process_state']=='running','metrics recover after SIGCONT')
    os.kill(frozen_pid, signal.SIGKILL)
    wait_for(lambda: status('st2')['process_state']=='exited','unexpected process exit reported')
    assert request('/healthz',auth=False)[0] == 503
    assert request('/api/instances/st2/start',{})[0] == 202
    wait_for(lambda: status('st2')['process_state']=='running','failed process can restart')
    print('PASS stale status blocks commands, process failure changes health, manual recovery',flush=True)

    # Fresh chunks are compatible with the existing analysis/export utility.
    chunks = sorted(Path('/home/video').glob('st1_chunk_*.bin'))
    assert chunks
    chunk = chunks[0]
    result = subprocess.run(['read_chunk_file','-I',str(chunk),'-a'],capture_output=True,text=True,check=True)
    stamps = [int(value) for value in re.findall(r'hdr.sendtime\s*[:=]?\s*(\d+)',result.stdout+result.stderr)]
    assert stamps, (result.stdout+result.stderr)[:1000]
    assert all(a<=b for a,b in zip(stamps,stamps[1:])), 'nonmonotonic timestamps'
    subprocess.run(['read_chunk_file','-I',str(chunk),'-O','/tmp/export.ts'],capture_output=True,check=True)
    assert Path('/tmp/export.ts').stat().st_size > 1316
    print('PASS read_chunk_file analysis, monotonic sendtime and TS export',flush=True)

    # UI configuration uses actual DB records; running streams cannot be rewritten.
    configuration = {'name':'st2','autostart':True,'msbuffer':120000,
        'source':{'host':'239.10.0.2','port':1234},
        'outputs':[{'name':'Edited program','host':'239.20.0.3','port':1234,'ttl':5,'rawudp':True},
                   {'name':'Added output','host':'239.20.0.4','port':1234,'ttl':5,'rawudp':False}]}
    assert request('/api/instances/st2/configuration',configuration)[0] == 409
    assert request('/api/instances/st2/stop',{})[0] == 200
    receivers.append(Receiver('239.20.0.4'))
    primary_id = status('st2')['outputs'][0]['id']
    assert request('/api/instances/st2/configuration',configuration)[0] == 200
    configured = status('st2')
    assert configured['outputs'][0]['id'] == primary_id
    assert configured['msbuffer']==120000 and len(configured['outputs'])==2
    assert request('/api/instances/st2/start',{})[0] == 202
    wait_for(lambda: status('st2')['process_state']=='running','configured instance starts')
    wait_for(lambda: receivers[-1].count()>20,'new output sends traffic')
    wait_for(lambda: len(receivers[2].packets[-1][1])==1316,'edited primary strips RTP')
    assert len(receivers[-1].packets[-1][1])==1328
    print('PASS configure stopped input/outputs, retain sink IDs, new multicast output, updated raw UDP',flush=True)

    # Removing a backup and changing the input are also persisted atomically.
    assert request('/api/instances/st2/stop',{})[0] == 200
    configuration['source']={'host':'239.10.0.22','port':2234}
    configuration['outputs']=configuration['outputs'][:1]
    configuration['autostart']=False
    assert request('/api/instances/st2/configuration',configuration)[0]==200
    assert status('st2')['source']['host']=='239.10.0.22'
    assert len(status('st2')['outputs'])==1
    new_config={'name':'st3','autostart':False,'msbuffer':180000,
        'source':{'host':'239.10.0.3','port':1234},
        'outputs':[{'name':'New program','host':'239.20.0.5','port':1234,'ttl':7,'rawudp':False}]}
    assert request('/api/instances',new_config)[0]==201
    assert status('st3')['process_state']=='stopped'
    assert request('/api/instances',new_config)[0]==409
    invalid=dict(new_config,name='st4',outputs=[{'name':'Collision','host':'239.20.0.1','port':1234}])
    assert request('/api/instances',invalid)[0]==409
    assert request('/api/instances',dict(new_config,name='bad',outputs=[]))[0]==400
    Path('/tmp/expected-configuration.json').write_text(json.dumps({'st2':configuration,'st3':new_config}))
    print('PASS input edit, output removal, UI instance creation, duplicate/invalid configuration rejection',flush=True)
finally:
    stop.set()
    for receiver in receivers:
        receiver.done=True
        receiver.thread.join(1)
        receiver.sock.close()
