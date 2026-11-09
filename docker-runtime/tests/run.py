#!/usr/bin/env python3
"""Isolated Docker integration test; never uses production config or volumes."""
import json
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
project = 'delayer-test-' + secrets.token_hex(4)

def run(*args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)

with tempfile.TemporaryDirectory(prefix='delayer-test-') as temporary:
    temp = Path(temporary)
    config = json.loads((ROOT/'docker-runtime/instances.example.json').read_text())
    config['instances'][1]['autostart'] = True
    (temp/'instances.json').write_text(json.dumps(config))
    # Named test volumes are scoped by a unique Compose project, then removed.
    override = {'services':{'delayer':{'environment':{'PGHOST':'database','PGPORT':'5432','TEST_BASEURL':'http://web:8080','DELAYER_MULTICAST_INTERFACE':'127.0.0.1'},'volumes':[
        {'type':'bind','source':str(temp/'instances.json'),'target':'/etc/delayer/instances.json','read_only':True},
        {'type':'bind','source':str(ROOT/'docker-runtime/tests'),'target':'/tests','read_only':True}]}}}
    (temp/'override.json').write_text(json.dumps(override))
    env = dict(os.environ, DB_PASSWORD=secrets.token_urlsafe(24), WEB_PASSWORD=secrets.token_urlsafe(24), WEB_PORT='0')
    # Reset host mode for isolated tests; UDP and the forwarding socket remain real.
    reset = temp/'bridge.yaml'
    reset.write_text('services:\n  delayer:\n    network_mode: !reset null\n    networks: [default]\n  database:\n    ports: !reset []\n')
    base = ['docker','compose','--project-directory',str(ROOT),'-p',project,'-f',str(ROOT/'compose.yaml'),'-f',str(ROOT/'compose.multicast-forward.yaml'),'-f',str(reset),'-f',str(temp/'override.json')]
    def compose(*args, **kwargs):
        return run(*base,*args,env=env,**kwargs)
    try:
        compose('up','-d','--build','--wait','database')
        # Force config IDs to differ from interface IDs, exposing historical bugs.
        sql = """INSERT INTO streaminterfaces (name) VALUES ('test-unused');
          INSERT INTO streaminterfaceconfigs (streaminterface_id,mode) VALUES (1,'unused'),(1,'unused');"""
        compose('exec','-T','database','psql','-U','delayer','-d','delayerdb','-v','ON_ERROR_STOP=1','-c',sql,stdout=subprocess.DEVNULL)
        compose('up','-d','--build','--wait','delayer','web')
        for address in ('invalid', '239.1.1.1', '192.0.2.1'):
            invalid = subprocess.run(['docker','run','--rm','-e','WEB_PASSWORD=startup-test',
                '-e','DELAYER_MULTICAST_INTERFACE='+address,'t-delayer:runtime'],
                capture_output=True,text=True,timeout=20)
            assert invalid.returncode != 0 and 'Invalid DELAYER_MULTICAST_INTERFACE' in invalid.stderr, invalid.stderr
        print('PASS invalid and unavailable multicast interfaces rejected at startup')
        compose('exec','-T','delayer','python3','-m','unittest','discover','-s','/tests','-p','test_*.py','-v')
        compose('exec','-T','delayer','python3','/tests/integration.py')
        compose('exec','-T','delayer','python3','/tests/password_change.py','change')
        # A second runtime against the same streams must fail ownership acquisition.
        network = project+'_default'
        original = compose('ps','-q','delayer',capture_output=True,text=True).stdout.strip()
        with (temp/'duplicate.env').open('w') as stream:
            stream.write(f'PGHOST=database\nPGPASSWORD={env["DB_PASSWORD"]}\nWEB_PASSWORD={env["WEB_PASSWORD"]}\nDELAYER_CONTROLLER_ID=delayer\n')
        duplicate = subprocess.run(['docker','run','--rm','--network',network,'--env-file',str(temp/'duplicate.env'),
            '-v',str(temp/'instances.json')+':/etc/delayer/instances.json:ro','t-delayer:runtime'],capture_output=True,text=True)
        assert duplicate.returncode != 0 and 'already owned' in duplicate.stderr, duplicate.stderr
        print('PASS duplicate container ownership rejected')
        # Intentional container restart retains data and starts each configured process.
        compose('restart','delayer')
        deadline = time.monotonic()+30
        while time.monotonic()<deadline:
            probe = subprocess.run(['docker','exec',original,'python3','/opt/delayer/control.py','--healthcheck'],capture_output=True)
            if probe.returncode==0:
                break
            time.sleep(.3)
        else:
            raise AssertionError('Runtime did not recover after container restart')
        print('PASS clean container stop/restart with persistent recording volume')
        compose('exec','-T','-e','TEST_LOGIN_PASSWORD=rotated-test-password','delayer','python3','/tests/verify_persistence.py')
        compose('exec','-T','delayer','python3','/tests/password_change.py','verify')
        compose('up','-d','--no-deps','--no-build','--force-recreate','--wait','delayer','web')
        compose('exec','-T','delayer','python3','/tests/password_change.py','verify')
        print('All Docker integration checks passed.')
    except BaseException:
        compose('logs','--tail','100')
        raise
    finally:
        compose('down','--volumes','--remove-orphans')
