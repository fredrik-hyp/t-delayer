"""Verify UI-created records and edited settings after a real container restart."""
import json
import os
from pathlib import Path

from auth_client import Client
client = Client()
previous_cookie = Path('/tmp/previous-session-cookie').read_text()
assert client.request('/api/status',auth=False,headers={'Cookie':previous_cookie})[0]==401
client.login()
code, data = client.request('/api/status')
assert code == 200
actual = {item['name']:item for item in data['instances']}
expected = json.loads(Path('/tmp/expected-configuration.json').read_text())
for name, config in expected.items():
    item = actual[name]
    assert item['autostart']==config['autostart']
    assert item['process_state']=='stopped'
    assert item['msbuffer']==config['msbuffer']
    assert all(item['source'][key]==value for key,value in config['source'].items())
    assert len(item['outputs'])==len(config['outputs'])
    for output, wanted in zip(item['outputs'],config['outputs']):
        assert all(output[key]==value for key,value in wanted.items())
print('PASS container restart expires old browser session and preserves UI configuration')
