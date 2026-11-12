#!/bin/sh
# Run inside a disposable matching Ubuntu container, never on a deployment host.
# Exercises real dpkg/debconf hooks; Docker/systemd are doubles (not PID 1 here).
set -eu
[ "${DELAYER_PACKAGE_TEST:-}" = isolated ] || { echo "Run only in the isolated test container" >&2; exit 2; }
PACKAGE=$1
mkdir -p /usr/local/bin /var/lib/t-delayer
cat > /usr/local/bin/systemctl <<'EOF'
#!/bin/sh
set -e
echo "$*" >> /tmp/systemctl.calls
case "$1:$2" in
    restart:t-delayer-docker.service) /usr/sbin/t-delayerctl start ;;
esac
EOF
cat > /usr/local/bin/docker <<'EOF'
#!/usr/bin/python3
import json
from pathlib import Path
import sys
import tarfile
args=sys.argv[1:]
state=Path('/tmp/fake-delayer-container')
if args[0]=='info':
    print(json.dumps({'OSType':'linux','SecurityOptions':[]}))
elif args[:2]==['image','inspect']:
    with tarfile.open('/usr/share/t-delayer/images.tar') as archive:
        manifest=json.load(archive.extractfile('manifest.json'))[0]
        payload=json.load(archive.extractfile(manifest['Config']))
    print(json.dumps([{'Os':payload['os'],'Architecture':payload['architecture'],
                      'Id':'sha256:'+'f'*64, 'Config':payload.get('config',{}),
                      'RootFS':{'Layers':payload['rootfs']['diff_ids']}}]))
elif args[:2]==['container','inspect']:
    if not state.exists():
        sys.stderr.write('No such container\n'); sys.exit(1)
    print(json.dumps([{'Config':{'Labels':{'io.polylegio.t-delayer.package':'t-delayer-docker'}},
                      'State':{'Status':'running','Health':{'Status':'healthy'}}}]))
elif args[:2]==['container','create']:
    Path('/tmp/fake-container.args').write_text(json.dumps(args))
    env_path=Path(args[args.index('--env-file')+1])
    public={key:value for key,value in (line.split('=',1) for line in env_path.read_text().splitlines())
            if key in ('WEB_BIND','WEB_PORT')}
    Path('/tmp/fake-web-env.json').write_text(json.dumps(public))
    state.touch()
elif args[:2]==['container','rm']:
    state.unlink(missing_ok=True)
elif args[:2]==['network','inspect']:
    print(json.dumps([{'Driver':'bridge','Labels':{'io.polylegio.t-delayer.package':'t-delayer-docker'}}]))
EOF
chmod 755 /usr/local/bin/systemctl /usr/local/bin/docker
export PATH=/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export DEBIAN_FRONTEND=noninteractive
# Ubuntu's minimal Docker base excludes documentation; a normal server does not.
for file in /etc/dpkg/dpkg.cfg.d/*; do
    sed -i '\|path-exclude.*/usr/share/doc|d' "$file"
done
dpkg --unpack "$PACKAGE"
python3 /tests/attended.py --initial
python3 - <<'PY'
import json
from pathlib import Path
config=json.loads(Path('/etc/t-delayer/config.json').read_text())
assert config['network_mode']=='bridge' and config['web_port']==18080
args=json.loads(Path('/tmp/fake-container.args').read_text())
assert args.count('create')==1
assert '127.0.0.1:18080:8080' in args
assert args[args.index('--restart')+1]=='on-failure'
assert not any('POSTGRES_PASSWORD=' in arg or 'WEB_PASSWORD=' in arg for arg in args)
assert Path('/usr/share/doc/t-delayer-docker/INSTALL.md').exists()
Path('/tmp/saved-credentials').write_bytes(Path('/var/lib/t-delayer/credentials.json').read_bytes())
PY
python3 /tests/attended.py
printf '%s\n' \
  't-delayer-docker t-delayer-docker/network-mode select host' \
  't-delayer-docker t-delayer-docker/web-bind string 0.0.0.0' \
  't-delayer-docker t-delayer-docker/web-port string 18081' | debconf-set-selections
dpkg-reconfigure -f noninteractive t-delayer-docker
python3 - <<'PY'
import json
from pathlib import Path
assert json.loads(Path('/etc/t-delayer/config.json').read_text())['network_mode']=='host'
assert json.loads(Path('/etc/t-delayer/config.json').read_text())['web_bind']=='0.0.0.0'
assert Path('/tmp/saved-credentials').read_bytes()==Path('/var/lib/t-delayer/credentials.json').read_bytes()
args=json.loads(Path('/tmp/fake-container.args').read_text())
assert args[args.index('--network')+1]=='host' and '--publish' not in args
for directory in ['video','postgresql','auth']:
    Path('/var/lib/t-delayer', directory, 'retention-sentinel').touch()
PY
dpkg --remove t-delayer-docker
test ! -e /tmp/fake-delayer-container
dpkg --purge t-delayer-docker
test ! -e /etc/t-delayer/config.json
test -f /var/lib/t-delayer/credentials.json
for directory in video postgresql auth; do
    test -f "/var/lib/t-delayer/$directory/retention-sentinel"
done
echo 'PASS real dpkg install, debconf preseed/reconfigure, host/bridge generation, removal and data retention (Docker/systemd doubled)'
