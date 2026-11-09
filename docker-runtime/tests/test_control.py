import copy
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, '/opt/delayer')
from control import APIError, load_config, parse_time, validate_command, validate_configuration, validate_import


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.now = datetime.now(timezone.utc).replace(tzinfo=None)
        self.stat = {'startpos': self.now - timedelta(minutes=5), 'lastpos': self.now}

    def test_time_requires_timezone_and_normalizes_utc(self):
        self.assertEqual(parse_time('2026-11-05T13:00:00+01:00', 'time'), datetime(2026,11,5,12))
        for value in ['2026-11-05T12:00:00', 'garbage', 17, '1960-01-01T00:00:00Z']:
            with self.assertRaises(APIError):
                parse_time(value, 'time')

    def test_jump_bounds_and_wall_clock_stop(self):
        self.assertRaises(APIError, validate_command, {'action':'jump'}, self.stat)
        self.assertRaises(APIError, validate_command, {'action':'jump', 'jumptopos':(self.now + timedelta(seconds=10)).isoformat()+'Z'}, self.stat)
        result = validate_command({'action':'jump', 'jumptopos':(self.now-timedelta(seconds=20)).isoformat()+'Z',
                                   'startplayat':(self.now+timedelta(seconds=20)).isoformat()+'Z',
                                   'playto':(self.now+timedelta(seconds=30)).isoformat()+'Z'}, self.stat)
        self.assertEqual(result['action'], 'jump')
        self.assertRaises(APIError, validate_command, {'action':'playto','playto':(self.now-timedelta(seconds=10)).isoformat()+'Z'}, self.stat)
        self.assertIsNone(validate_command({'action':'playto'}, self.stat)['playto'])

    def test_unknown_action_and_fields_are_rejected(self):
        for body in [{'action':'restart'}, {'action':'pause','pid':1}, {'action':'play','jumptopos':'2026-11-05T12:00:00Z'}]:
            self.assertRaises(APIError, validate_command, body, self.stat)

    def test_ui_configuration_requires_input_outputs_and_rejects_unknown_fields(self):
        self.assertRaises(ValueError, validate_configuration, {'name':'st1'})
        item = {'name':'st1','source':{'host':'239.10.0.1','port':1234},
                'outputs':[{'name':'Program','host':'239.20.0.1','port':1234}]}
        self.assertFalse(validate_configuration(item)['autostart'])
        self.assertRaises(ValueError, validate_configuration, dict(item,command='sh'))

    def test_config_rejects_unsafe_names_duplicate_destinations_and_too_many_outputs(self):
        valid = {'instances':[{'name':'st1','source':{'host':'239.10.0.1','port':1234},
                              'outputs':[{'host':'239.20.0.1','port':1234}]}]}
        def load(value):
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'config.json'
                path.write_text(json.dumps(value))
                return load_config(path)
        self.assertEqual(load(valid)[0]['name'], 'st1')
        self.assertEqual(load({'instances':[]}), [])
        for name in ['../bad','-bad','x'*41]:
            value = copy.deepcopy(valid)
            value['instances'][0]['name'] = name
            self.assertRaises(ValueError, load, value)
        value = copy.deepcopy(valid)
        value['instances'][0]['outputs'] *= 2
        self.assertRaises(ValueError, load, value)
        value['instances'][0]['outputs'] *= 5
        self.assertRaises(ValueError, load, value)
        value = copy.deepcopy(valid)
        value['instances'][0]['source']['host'] = '127.0.0.1'
        self.assertRaises(ValueError, load, value)
        self.assertEqual(load({'instances':[{'name':'existing'}]})[0]['name'], 'existing')

    def test_import_accepts_single_or_collection_and_requires_complete_portable_settings(self):
        item = {'name':'st1', 'source':{'host':'239.10.0.1','port':1234},
                'outputs':[{'name':'Program','host':'239.20.0.1','port':1234,'ttl':7,'rawudp':True}]}
        self.assertEqual(validate_import(item), validate_import({'instances':[item]}))
        self.assertEqual(validate_import({'instances':[]}), [])
        self.assertEqual(validate_import({'instances':[item]}, 'st1')[0]['name'], 'st1')
        invalid = [[], None, {'instances':{}}, {'instances':[{'name':'st1'}]},
                   {'instances':[item], 'extra':1}, {'instances':[item,item]},
                   dict(item,source=dict(item['source'],command='sh')),
                   dict(item,outputs=[dict(item['outputs'][0],id=42)]),
                   dict(item,autostart=1), dict(item,msbuffer=0)]
        for body in invalid:
            with self.subTest(body=body), self.assertRaises(ValueError):
                validate_import(body)
        for body in [item, {'instances':[]}, {'instances':[item,dict(item,name='st2',outputs=[{'host':'239.20.0.2','port':1234}])]}]:
            self.assertRaises(ValueError, validate_import, body, 'st2')


if __name__ == '__main__':
    unittest.main()
