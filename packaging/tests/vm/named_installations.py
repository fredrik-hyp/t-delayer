"""Named installation checks executed only inside the disposable Ubuntu guest."""
import hashlib
import http.cookiejar
import urllib.request
import json
import shutil
from pathlib import Path
import subprocess
import sys
import time

NAMES = ('alpha','beta')
ROOT = Path('/root/named-installation-evidence.json')


def run(*args):
    result=subprocess.run(args,capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError('Command failed: '+repr(args)+'\n'+result.stdout+result.stderr)
    return result.stdout.strip()


def inspect(name):
    return json.loads(run('docker','inspect','t-delayer-'+name))[0]


def credentials(name):
    state = Path('/var/lib/t-delayer/installations')/name
    return {key:hashlib.sha256((state/path).read_bytes()).hexdigest()
            for key,path in [('credentials','credentials.json'),('auth','auth/auth.json')]}


def database(name,sql):
    return run('docker','exec','t-delayer-'+name,'psql','-U','delayer','-d','delayerdb','-Atc',sql)


def verify_saved(saved):
    for name in NAMES:
        assert credentials(name)==saved['credentials'][name]
        assert Path('/srv/delayer-video',name,'sentinel').read_text()==name
        assert database(name,'SELECT value FROM named_sentinel')==name
        assert inspect(name)['State']['Health']['Status']=='healthy'


assert Path('/var/lib/delayer-vm-cloud-ready').is_file(), 'Run only inside the disposable test guest'
assert run('lsblk','-ndo','SERIAL','/dev/disk/by-id/virtio-delayer-video-test')=='delayer-video-test'
phase = sys.argv[1]
if phase == 'prepare':
    # These names/data belong only to this disposable guest. Reset failed prior
    # test attempts so --resume exercises creation again, never production data.
    for name in NAMES:
        config=Path('/etc/t-delayer/installations')/name
        state=Path('/var/lib/t-delayer/installations')/name
        if (config/'config.json').exists():
            run('t-delayerctl','stop',name)
            run('systemctl','disable','t-delayer@'+name+'.service')
            run('t-delayerctl','remove-containers',name)
            shutil.rmtree(config);shutil.rmtree(state)
            shutil.rmtree(Path('/srv/delayer-video')/name,ignore_errors=True)
            shutil.rmtree(Path('/etc/systemd/system')/('t-delayer@'+name+'.service.d'),ignore_errors=True)
    # Keep all three recording trees as siblings on the same disposable disk.
    run('t-delayerctl','configure','default','--video-path','/srv/delayer-video/default')
    run('t-delayerctl','restart','default')
    default_id = run('docker','inspect','--format','{{.Id}}','t-delayer')
    for name,port in zip(NAMES,(18081,18082)):
        run('t-delayerctl','create',name,'--web-port',str(port),'--video-path','/srv/delayer-video/'+name)
        run('systemctl','is-active','t-delayer@'+name+'.service')
        run('systemctl','is-enabled','t-delayer@'+name+'.service')
        Path('/srv/delayer-video',name,'sentinel').write_text(name)
        database(name,"CREATE TABLE named_sentinel(value text); INSERT INTO named_sentinel VALUES ('"+name+"')")
        obj = inspect(name)
        assert obj['Config']['Labels']['io.polylegio.t-delayer.installation']==name
        assert 'DELAYER_CONTROLLER_ID=t-delayer-docker-'+name in obj['Config']['Env']
    assert run('docker','inspect','--format','{{.Id}}','t-delayer')==default_id
    # Browser cookies are shared across ports: unique controller IDs must also
    # keep all three installations logged in within the same browser cookie jar.
    jar=http.cookiejar.CookieJar()
    opener=urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))
    def api(port,path,body=None):
        request=urllib.request.Request('http://127.0.0.1:'+str(port)+path,
            data=json.dumps(body).encode() if body is not None else None,
            headers={'Content-Type':'application/json','X-Delayer-Control':'1'} if body is not None else {})
        with opener.open(request,timeout=30) as response:return json.load(response)
    for name,port in ((None,8080),('alpha',18081),('beta',18082)):
        directory=Path('/var/lib/t-delayer') if name is None else Path('/var/lib/t-delayer/installations')/name
        password=json.loads((directory/'credentials.json').read_text())['web_password']
        api(port,'/api/auth/login',{'username':'admin','password':password})
    assert len({cookie.name for cookie in jar})==3
    assert all(api(port,'/api/auth/session')['authenticated'] for port in (8080,18081,18082))
    api(18081,'/api/auth/logout',{})
    assert all(api(port,'/api/auth/session')['authenticated'] for port in (8080,18082))
    for port in (8080,18082):api(port,'/api/auth/logout',{})
    saved = {'credentials':{name:credentials(name) for name in NAMES}}
    beta_id = inspect('beta')['Id']
    alpha_id = inspect('alpha')['Id']
    run('t-delayerctl','restart','alpha')
    assert inspect('alpha')['Id']!=alpha_id
    assert inspect('beta')['Id']==beta_id
    assert run('docker','inspect','--format','{{.Id}}','t-delayer')==default_id
    run('t-delayerctl','stop','alpha')
    assert inspect('alpha')['State']['Status']=='exited'
    assert inspect('beta')['State']['Status']=='running'
    run('t-delayerctl','start','alpha')
    verify_saved(saved)
    saved['checks']={'create_enable_start':True,'scoped_restart_stop_start':True,'legacy_container_unchanged':True,'shared_browser_cookie_isolation':True}
    ROOT.write_text(json.dumps(saved,indent=2)+'\n')
    print('PASS named units start, stop and restart through real systemd without changing peer/default containers')
elif phase == 'blocked':
    for name in NAMES:
        assert inspect(name)['State']['Running'] is False
        assert run('systemctl','show','-p','ActiveState','--value','t-delayer@'+name+'.service')!='active'
        assert not Path('/srv/delayer-video',name).exists()
    print('PASS both named services wait for the recording mount')
elif phase == 'verify':
    deadline=time.monotonic()+420
    for name in NAMES:
        while time.monotonic()<deadline:
            result=subprocess.run(['systemctl','is-active','--quiet','t-delayer@'+name+'.service'])
            if result.returncode==0:break
            time.sleep(1)
        else:raise AssertionError('Named service did not start after mount release: '+name)
    saved=json.loads(ROOT.read_text());verify_saved(saved)
    saved['checks'].update(boot_after_recording_mount=True,persistence_after_reboot=True)
    ROOT.write_text(json.dumps(saved,indent=2)+'\n')
    print('PASS named databases, auth and recordings persist and start automatically after reboot/mount release')
elif phase == 'remove':
    saved=json.loads(ROOT.read_text())
    run('apt-get','remove','-y','t-delayer-docker')
    for container in ('t-delayer','t-delayer-alpha','t-delayer-beta'):
        assert subprocess.run(['docker','container','inspect',container],capture_output=True).returncode!=0
    run('apt-get','purge','-y','t-delayer-docker')
    for name in NAMES:
        assert credentials(name)==saved['credentials'][name]
        assert Path('/srv/delayer-video',name,'sentinel').read_text()==name
        assert not Path('/etc/t-delayer/installations',name,'config.json').exists()
        assert not Path('/etc/systemd/system','t-delayer@'+name+'.service.d','storage.conf').exists()
    assert Path('/var/lib/t-delayer/credentials.json').exists()
    saved['checks'].update(package_removal_stops_all=True,purge_retains_all_data=True)
    ROOT.write_text(json.dumps(saved,indent=2)+'\n')
    print('PASS real package removal/purge stops all services and retains default/named persistent data')
else:
    raise SystemExit('Unknown phase')
