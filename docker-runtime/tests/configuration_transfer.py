"""JSON transfer through real nginx, authenticated API and PostgreSQL."""
import copy
import json
from pathlib import Path

import psycopg2

from auth_client import Client

client = Client()
request = client.request
assert request('/api/configuration', auth=False)[0] == 401
assert request('/api/configuration/import', {'instances':[]}, auth=False)[0] == 401
client.login()


def export(name=None):
    code, data = request(f'/api/instances/{name}/configuration' if name else '/api/configuration')
    assert code == 200, (code, data)
    return data


def config(name, group):
    return {'name':name, 'autostart':False, 'msbuffer':60000,
            'source':{'host':'239.80.0.1', 'port':1234},
            'outputs':[{'name':'Imported program', 'host':group, 'port':1234, 'ttl':3, 'rawudp':True}]}


original = export()
assert {item['name'] for item in original['instances']} == {'st1','st2','st3'}
for item in original['instances']:
    assert export(item['name']) == item
    assert set(item) == {'name','autostart','msbuffer','source','outputs'}
    assert set(item['source']) == {'host','port'}
    for output in item['outputs']:
        assert set(output) == {'name','host','port','ttl','rawudp'}
assert request('/api/instances/missing/configuration')[0] == 404
assert request('/api/instances/missing/configuration/import', config('missing','239.80.1.1'))[0] == 404
assert request('/api/configuration/import', original, control=False)[0] == 403
assert client.request('/api/configuration/import', original, headers={'Origin':'https://other.example'})[0] == 403
assert request('/api/configuration/import', original)[0] == 409  # st1 is running
assert export() == original
assert request('/api/configuration/import', {'instances':[]}) == (200, {'saved':True, 'imported':[]})

st3 = export('st3')
edited = copy.deepcopy(st3)
edited['msbuffer'] = 90000
edited['outputs'].append({'name':'Extra', 'host':'239.80.1.2', 'port':2234, 'ttl':2, 'rawudp':False})
assert request('/api/instances/st3/configuration/import', dict(edited,name='different'))[0] == 400
assert request('/api/instances/st3/configuration/import', {'instances':[edited]})[0] == 200
assert export('st3') == edited
assert request('/api/instances/st3/configuration/import', st3)[0] == 200
assert export('st3') == st3

# A late DB conflict must roll back earlier updates and inserts, including
# controller membership, and release locks for every uncommitted new instance.
new = config('import-rolled-back', '239.80.1.3')
conflict = config('test-unused', '239.80.1.4')  # created by the test runner in DB only
assert request('/api/configuration/import', {'instances':[edited,new,conflict]})[0] == 409
assert export() == original
with psycopg2.connect() as connection:
    with connection.cursor() as cur:
        cur.execute('SELECT id FROM streaminterfaces WHERE name=%s', (new['name'],))
        assert cur.fetchone() is None
        cur.execute('SELECT pg_try_advisory_lock(hashtextextended(%s,0))', ('delayer:'+new['name'],))
        assert cur.fetchone()[0], 'Failed import leaked an ownership lock'
        cur.execute('SELECT pg_advisory_unlock(hashtextextended(%s,0))', ('delayer:'+new['name'],))

invalid = [dict(edited,outputs=export('st1')['outputs']), {'instances':[edited,edited]},
           {'instances':[edited,{'name':'incomplete'}]}, dict(edited,msbuffer='90000')]
for body in invalid:
    assert request('/api/configuration/import', body)[0] in (400,409)
    assert export() == original

# Batch validation uses final destinations, so two stopped instances can swap
# outputs without a temporary collision. Instances omitted from the file stay.
st2 = export('st2')
swap2, swap3 = copy.deepcopy(st2), copy.deepcopy(st3)
swap2['outputs'], swap3['outputs'] = swap3['outputs'], swap2['outputs']
assert request('/api/configuration/import', {'instances':[swap2,swap3]})[0] == 200
assert export('st2') == swap2 and export('st3') == swap3
assert request('/api/configuration/import', {'instances':[st2,st3]})[0] == 200
assert export() == original

# One-object create, round trip, and a batch larger than the old 16 KiB limit.
single = config('import-single', '239.80.1.5')
assert request('/api/configuration/import', single)[0] == 200
assert export('import-single') == single
batch = []
for index in range(16):
    item = config(f'import-batch-{index}', f'239.81.{index}.1')
    item['outputs'] = [dict(item['outputs'][0],host=f'239.81.{index}.{path+1}',name='å'*100) for path in range(8)]
    batch.append(item)
body = {'instances':batch}
assert len(json.dumps(body).encode()) > 16384
code, result = request('/api/configuration/import', body)
assert code == 200 and len(result['imported']) == len(batch), (code, result)
assert request('/api/configuration/import', export())[0] == 409  # running st1 still protected
assert request('/api/configuration/import', {'instances':[export(item['name']) for item in batch]})[0] == 200
actual = {item['name']:item for item in request('/api/status')[1]['instances']}
for item in [single] + batch:
    assert export(item['name']) == item
    assert actual[item['name']]['pid'] is None
    assert actual[item['name']]['process_state'] == 'stopped'
expected_path = Path('/tmp/expected-configuration.json')
expected = json.loads(expected_path.read_text())
expected.update({item['name']:item for item in [single] + batch})
expected_path.write_text(json.dumps(expected))
print('PASS JSON single/all export, import create/update, round trip, atomic rollback, locks, collisions, large batch and session guards', flush=True)
