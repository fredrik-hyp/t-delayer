#!/usr/bin/env python3
"""Run a legacy/default install and two named installs against real Docker.

Uses unique test container names, networks and temporary bind directories.
Only systemd is substituted; the actual lifecycle helper, databases and APIs run.
"""
import argparse
from contextlib import redirect_stdout
import http.cookiejar
import importlib.util
import io
import json
import os
from pathlib import Path
import secrets
import socket
import subprocess
import tempfile
import urllib.error
import urllib.request
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('multi_ctl', ROOT/'packaging/t-delayerctl.py')
ctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ctl)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image', required=True, help='A packaged amd64 image with internal PostgreSQL')
args = parser.parse_args()
suffix = secrets.token_hex(4)
names = [None, 'qa-'+suffix+'-news', 'qa-'+suffix+'-sport']
ports = {}
for name in names:
    with socket.socket() as probe:
        probe.bind(('127.0.0.1',0))
        ports[name] = probe.getsockname()[1]
assert len(set(ports.values()))==3
original_paths = ctl.installation_paths

def paths(name):
    result = original_paths(name)
    if result['INSTALLATION'] is None:
        result.update(CONTAINER='delayer-legacy-test-'+suffix, NETWORK='delayer-legacy-test-'+suffix)
    return result


def client(name, password=None):
    info = paths(name)
    saved = json.loads((info['STATE_DIR']/'credentials.json').read_text())
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    def api(path, body=None):
        request = urllib.request.Request('http://127.0.0.1:'+str(ports[name])+path,
                    data=json.dumps(body).encode() if body is not None else None,
                    headers={'Content-Type':'application/json','X-Delayer-Control':'1'} if body is not None else {})
        with opener.open(request,timeout=35) as response:
            return json.load(response)
    api('/api/auth/login',{'username':'admin','password':password or saved['web_password']})
    return api


def container_id(name):
    return ctl.inspect('container',paths(name)['CONTAINER'])['Id']


with tempfile.TemporaryDirectory(prefix='delayer-multiple-test-') as temporary:
    root = Path(temporary).resolve()
    ctl.CONFIG_ROOT = root/'etc'
    ctl.STATE_ROOT = root/'state'
    ctl.SYSTEMD_ROOT = root/'systemd'
    ctl.SHARE_DIR = root/'share'
    ctl.LOCK_PATH = root/'registry.lock'
    ctl.SHARE_DIR.mkdir()
    image = ctl.inspect('image',args.image)
    (ctl.SHARE_DIR/'images.json').write_text(json.dumps({'architecture':'amd64','runtime_id':image['Id'],
        'runtime':args.image,'runtime_fingerprint':ctl.image_fingerprint(image)}))
    with patch.object(ctl,'installation_paths',side_effect=paths), patch.dict(os.environ,{key:value for key,value in os.environ.items() if not key.startswith('DELAYER_')},clear=True):
        try:
            before_credentials = {}
            for name in names:
                with ctl.using_installation(name):
                    ctl.configure({'network_mode':'bridge','web_port':ports[name],'multicast_interface':'127.0.0.1'})
                    Path(ctl.configuration()['video_path']).mkdir(parents=True,exist_ok=True)
                    (Path(ctl.configuration()['video_path'])/'recording-sentinel').write_text(name or 'default')
                    before_credentials[name]=(ctl.STATE_DIR/'credentials.json').read_bytes()
                    ctl.start()
                api = client(name)
                assert api('/api/configuration')=={'instances':[]}
                index = names.index(name)+1
                api('/api/instances',{'name':'shared-name','autostart':True,'msbuffer':60000,
                    'source':{'host':f'239.251.90.{index}','port':1234},
                    'outputs':[{'name':name or 'default','host':f'239.251.91.{index}','port':1234,'ttl':1,'rawudp':True}]})
                api('/api/instances/shared-name/start',{})
                api('/api/auth/logout',{})
            print('PASS default plus two named containers run concurrently with independent databases and same stream name',flush=True)
            ids={name:container_id(name) for name in names}
            sport=names[2]
            api=client(sport)
            api('/api/auth/password',{'current_password':json.loads(before_credentials[sport])['web_password'],
                'new_password':'isolated-sport-password','confirm_password':'isolated-sport-password'})
            api('/api/auth/logout',{})
            for name in names[:2]:
                api=client(name);assert api('/api/configuration')['instances'][0]['outputs'][0]['name']==(name or 'default');api('/api/auth/logout',{})
            with ctl.using_installation(sport):
                ctl.start()  # Actual Docker replacement of only the selected installation.
            assert container_id(sport)!=ids[sport]
            assert all(container_id(name)==ids[name] for name in names[:2])
            api=client(sport,'isolated-sport-password')
            assert api('/api/configuration')['instances'][0]['name']=='shared-name'
            api('/api/auth/logout',{})
            for name in names:
                info=paths(name)
                assert (info['STATE_DIR']/'credentials.json').read_bytes()==before_credentials[name]
                assert (info['STATE_DIR']/'video'/'recording-sentinel').read_text()==(name or 'default')
            print('PASS selected recreation preserves credentials, changed password, recording sentinel and configuration; other container IDs unchanged',flush=True)
            with ctl.using_installation(sport):
                before=(ctl.CONFIG_DIR/'config.json').read_bytes()
                try:
                    ctl.configure({'web_port':ports[names[1]]})
                except ValueError as error:
                    assert 'conflicts' in str(error)
                else:
                    raise AssertionError('Expected duplicate port rejection')
                assert (ctl.CONFIG_DIR/'config.json').read_bytes()==before
                ctl.remove_container()
            assert all(container_id(name)==ids[name] for name in names[:2])
            print('PASS conflicts rejected before writes; removing one container leaves both other installations running',flush=True)
            output=io.StringIO()
            with redirect_stdout(output):ctl.list_installations()
            assert all((name or 'default') in output.getvalue() for name in names)
            assert 'password' not in output.getvalue().lower()
            print('All multiple-installation Docker checks passed.',flush=True)
        except BaseException:
            for name in names:
                result=ctl.docker('logs','--tail','25',paths(name)['CONTAINER'],optional=True)
                print(result.stdout+result.stderr,flush=True)
            raise
        finally:
            for name in reversed(names):
                with ctl.using_installation(name):
                    ctl.remove_container()
                    if ctl.check_owned('network',ctl.NETWORK):ctl.docker('network','rm',ctl.NETWORK)
