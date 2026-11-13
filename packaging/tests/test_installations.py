"""Named installations must isolate identities, persistent data and lifecycle actions."""
from contextlib import redirect_stdout
import fcntl
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import socket
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('named_ctl', ROOT / 'packaging/t-delayerctl.py')
ctl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ctl)
REAL_VALIDATE_LISTENER = ctl.validate_listener


class InstallationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for key, value in dict(CONFIG_ROOT=self.root/'etc', STATE_ROOT=self.root/'state',
                               SYSTEMD_ROOT=self.root/'systemd', LOCK_PATH=self.root/'registry.lock').items():
            override = patch.object(ctl, key, value)
            override.start()
            self.addCleanup(override.stop)
        override = patch.dict(os.environ, {}, clear=True)
        override.start()
        self.addCleanup(override.stop)
        self.driver = patch.object(ctl, 'validate_docker', side_effect=lambda config: config['network_mode'])
        self.driver.start()
        self.addCleanup(self.driver.stop)
        override = patch.object(ctl, 'validate_listener')
        self.listener = override.start()
        self.addCleanup(override.stop)
        self.configure(None, 8080)

    def configure(self, name, port, **options):
        with ctl.using_installation(name):
            ctl.configure(dict(web_port=port, **options))
            return ctl.configuration()

    def test_existing_paths_and_identity_remain_unchanged(self):
        with patch.object(ctl, 'CONFIG_ROOT', Path('/etc/t-delayer')), patch.object(ctl, 'STATE_ROOT', Path('/var/lib/t-delayer')), \
                patch.object(ctl, 'SYSTEMD_ROOT', Path('/etc/systemd/system')):
            expected = dict(INSTALLATION=None, CONFIG_DIR=Path('/etc/t-delayer'), STATE_DIR=Path('/var/lib/t-delayer'),
                            DROPIN_DIR=Path('/etc/systemd/system/t-delayer-docker.service.d'), SERVICE='t-delayer-docker.service',
                            CONTAINER='t-delayer', NETWORK='t-delayer-bridge', CONTROLLER_ID='t-delayer-docker')
            self.assertEqual(ctl.installation_paths(None), expected)
            self.assertEqual(ctl.installation_paths('default'), expected)

    def test_names_cannot_escape_paths_or_systemd_instance_names(self):
        for name in ('', '../bad', 'a/b', '-bad', 'two words', 'bad.service', 'UPPER', '%i', 'a'*41):
            with self.subTest(name=name), self.assertRaises(ValueError):
                ctl.installation_paths(name)
        self.assertEqual(ctl.installation_paths('news-1')['CONTAINER'], 't-delayer-news-1')

    def test_names_get_separate_secrets_bootstrap_mounts_and_controller_ids(self):
        default = ctl.installation_paths(None)
        saved = (default['STATE_DIR']/'credentials.json').read_bytes()
        self.configure('news', 9010)
        self.configure('sport', 9020)
        seen = set()
        for name in (None, 'news', 'sport'):
            paths = ctl.installation_paths(name)
            seen.add((paths['STATE_DIR']/'credentials.json').read_bytes())
            self.assertEqual((paths['CONFIG_DIR']/'config.json').stat().st_mode & 0o777, 0o600)
            self.assertTrue((paths['CONFIG_DIR']/'instances.json').exists())
            self.assertIn(str(paths['STATE_DIR']/'video'), (paths['DROPIN_DIR']/'storage.conf').read_text())
        self.assertEqual(len(seen), 3)
        self.assertEqual((default['STATE_DIR']/'credentials.json').read_bytes(), saved)
        self.assertEqual(ctl.installations(), [None, 'news', 'sport'])

    def test_named_reconfiguration_preserves_unmentioned_settings_and_credentials(self):
        original = self.configure('news', 9010, video_path=str(self.root/'recordings'), network_mode='bridge')
        with ctl.using_installation('news'):
            saved = (ctl.STATE_DIR/'credentials.json').read_bytes()
            ctl.configure({'web_bind': '0.0.0.0'})
            expected = dict(original, web_bind='0.0.0.0')
            self.assertEqual(ctl.configuration(), expected)
            self.assertEqual((ctl.STATE_DIR/'credentials.json').read_bytes(), saved)

    def test_listener_conflicts_include_stopped_installations_and_wildcard_binds(self):
        self.configure('news', 9010, web_bind='0.0.0.0')
        with self.assertRaisesRegex(ValueError, 'Web listener conflicts'):
            self.configure('sport', 9010)
        self.assertFalse((ctl.installation_paths('sport')['CONFIG_DIR']/'config.json').exists())
        # Distinct explicit addresses may use the same port.
        self.configure('news', 9010, web_bind='192.0.2.11')
        self.configure('sport', 9010, web_bind='192.0.2.10')
        with self.assertRaisesRegex(ValueError, 'Web listener conflicts'):
            self.configure(None, 9010, web_bind='0.0.0.0')

    def test_recordings_never_overlap_other_recordings_or_private_database_and_auth(self):
        first = self.configure('news', 9010)
        for path in (first['video_path'], str(Path(first['video_path'])/'child'),
                     str(ctl.installation_paths('news')['STATE_DIR']),
                     str(ctl.installation_paths('sport')['STATE_DIR']/'auth')):
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, 'directory|storage'):
                self.configure('sport', 9020, video_path=path)
        alias = self.root/'alias'
        alias.symlink_to(first['video_path'])
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            self.configure('sport', 9020, video_path=str(alias))

    def test_static_ip_conflicts_are_rejected_but_separate_macvlan_addresses_share_web_port(self):
        self.driver.stop()
        self.driver = patch.object(ctl, 'validate_docker', return_value='macvlan')
        self.driver.start()
        self.addCleanup(self.driver.stop)
        with patch.object(ctl, 'network_driver', side_effect=lambda config: 'host' if config['network_mode']=='host' else 'macvlan'):
            self.configure('news', 9010, network_mode='existing', network_name='lan', container_ip='192.0.2.10', web_bind='0.0.0.0')
            self.configure('sport', 9010, network_mode='existing', network_name='lan', container_ip='192.0.2.11', web_bind='0.0.0.0')
            with self.assertRaisesRegex(ValueError, 'Container IP conflicts'):
                self.configure('sport', 9010, network_mode='existing', network_name='lan', container_ip='192.0.2.10', web_bind='0.0.0.0')

    def test_named_objects_require_matching_installation_ownership(self):
        obj = {'Config': {'Labels': {ctl.LABEL: ctl.OWNER}}}
        with patch.object(ctl, 'inspect', return_value=obj):
            with ctl.using_installation(None):
                self.assertEqual(ctl.check_owned('container', ctl.CONTAINER), obj)
            with ctl.using_installation('news'), self.assertRaisesRegex(RuntimeError, 'another installation'):
                ctl.remove_container()
            obj['Config']['Labels'][ctl.INSTALLATION_LABEL] = 'news'
            with ctl.using_installation('sport'), self.assertRaisesRegex(RuntimeError, 'another installation'):
                ctl.remove_container()

    def test_named_start_creates_only_its_own_container_network_and_mounts(self):
        self.configure('news', 9010, network_mode='bridge')
        calls, env = [], {}
        def docker(*args, **kwargs):
            calls.append(args)
            if args[:2] == ('container', 'create'):
                env.update(line.split('=',1) for line in Path(args[args.index('--env-file')+1]).read_text().splitlines())
            return subprocess.CompletedProcess(args, 0, stdout='', stderr='')
        with ctl.using_installation('news'), patch.object(ctl, 'docker', side_effect=docker), \
                patch.object(ctl, 'check_owned', return_value=None), patch.object(ctl, 'bundled_image', return_value={'Id':'trusted-image'}), \
                patch.object(ctl, 'inspect', return_value={'State':{'Status':'running','Health':{'Status':'healthy'}}}):
            ctl.start()
        create = next(call for call in calls if call[:2] == ('container','create'))
        self.assertEqual(create[create.index('--name')+1], 't-delayer-news')
        self.assertEqual(create[create.index('--network')+1], 't-delayer-news-bridge')
        self.assertIn(ctl.INSTALLATION_LABEL+'=news', create)
        self.assertIn(str(ctl.installation_paths('news')['STATE_DIR']/'postgresql'), ' '.join(create))
        self.assertEqual(env['DELAYER_CONTROLLER_ID'], 't-delayer-docker-news')
        self.assertFalse(any(call[:2] in (('container','stop'),('container','rm')) for call in calls))

    def assert_lock_is_free(self, *args):
        with ctl.LOCK_PATH.open('w') as stream:
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        return subprocess.CompletedProcess(args, 0)

    def test_create_does_not_hold_registry_lock_while_systemd_starts_helper(self):
        with ctl.using_installation('news'), patch.object(ctl, 'inspect', return_value=None), \
                patch.object(ctl, 'load_images'), patch.object(ctl, 'systemctl', side_effect=self.assert_lock_is_free) as systemctl:
            ctl.create({'web_port':9010}, no_start=True)
            self.assertIn(('enable','t-delayer@news.service'), [call.args for call in systemctl.call_args_list])
            self.assertNotIn(('start','t-delayer@news.service'), [call.args for call in systemctl.call_args_list])
            saved = (ctl.STATE_DIR/'credentials.json').read_bytes()
            with self.assertRaisesRegex(ValueError, 'already exists'):
                ctl.create({'web_port':9020})
            self.assertEqual((ctl.STATE_DIR/'credentials.json').read_bytes(), saved)

    def test_automatic_port_selection_skips_managed_reservations(self):
        self.configure('news', 8081)
        with ctl.using_installation('sport'), patch.object(ctl.socket, 'socket'):
            self.assertEqual(ctl.available_port('127.0.0.1'), 8082)

    def test_listener_probe_rejects_active_ports_but_allows_restart_after_http_time_wait(self):
        server=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        server.bind(('127.0.0.1',0));server.listen(1)
        config=dict(network_mode='host',web_bind='127.0.0.1',web_port=server.getsockname()[1])
        with patch.object(ctl,'check_owned',return_value=None):
            with self.assertRaisesRegex(ValueError,'unavailable'):
                REAL_VALIDATE_LISTENER(config,'host')
            try:
                with socket.create_connection(server.getsockname()) as client:
                    connection,_=server.accept()
                    connection.sendall(b'HTTP/1.0 200 OK\r\n\r\n')
                    connection.close()  # Server actively closes; its port enters TIME_WAIT.
                    while client.recv(1024):
                        pass
            finally:
                server.close()
            REAL_VALIDATE_LISTENER(config,'host')

    def test_public_start_uses_systemd_and_internal_start_keeps_the_selected_scope(self):
        self.configure('news',9010)
        with patch.object(ctl.os,'geteuid',return_value=0), patch.object(ctl,'systemctl',side_effect=self.assert_lock_is_free) as systemctl, \
                patch('sys.argv',['t-delayerctl','start','news']):
            ctl.main()
            systemctl.assert_called_once_with('start','t-delayer@news.service')
        called=[]
        with patch.object(ctl.os,'geteuid',return_value=0), patch.object(ctl,'start',side_effect=lambda:called.append(ctl.CONTAINER)), \
                patch('sys.argv',['t-delayerctl','start','news','--direct']):
            ctl.main()
        self.assertEqual(called,['t-delayer-news'])

    def test_list_shows_profiles_without_reading_or_printing_credentials(self):
        self.configure('news',9010)
        output=io.StringIO()
        with redirect_stdout(output), patch.object(ctl,'check_owned',return_value=None):
            ctl.list_installations()
        self.assertIn('default\tt-delayer\t',output.getvalue())
        self.assertIn('news\tt-delayer-news\t',output.getvalue())
        self.assertNotIn('password',output.getvalue().lower())


if __name__=='__main__':
    unittest.main()
