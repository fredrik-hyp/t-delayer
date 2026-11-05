"""Verify UI-created records and edited settings after a real container restart."""
import base64
import json
import os
from pathlib import Path
import urllib.request

auth = base64.b64encode(('admin:' + os.environ['WEB_PASSWORD']).encode()).decode()
request = urllib.request.Request(os.environ.get('TEST_BASEURL','http://127.0.0.1:8080')+'/api/status', headers={'Authorization':'Basic '+auth})
with urllib.request.urlopen(request, timeout=10) as response:
    actual = {item['name']:item for item in json.load(response)['instances']}
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
print('PASS UI-created instance, input/output edits and autostart survive container restart')
