from http.cookies import SimpleCookie
import hashlib
import json
import sys
import unittest
from pathlib import Path
import tempfile
from unittest.mock import patch

sys.path.insert(0, '/opt/delayer')
from web_auth import AuthError, Sessions, SESSION_SECONDS, make_credentials, store_credentials, initialize_credentials


class SessionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.credentials = make_credentials('admin', 'session-test-password')

    def setUp(self):
        self.now = 1000
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.path = Path(temporary.name) / 'auth.json'
        store_credentials(self.path, self.credentials)
        self.auth = Sessions(self.credentials, 'test-controller', clock=lambda: self.now, credential_path=self.path)

    def cookie(self, token):
        return self.auth.cookie_name + '=' + token

    def test_password_is_hashed_and_wrong_credentials_are_rejected(self):
        self.assertNotIn('session-test-password', json.dumps(self.credentials))
        for username, password in [('admin','wrong'),('other','session-test-password'),('\ud800','password'),('admin',None)]:
            with self.assertRaises(AuthError) as error:
                self.auth.login(username,password,'client')
            self.assertEqual(error.exception.status,401)

    def test_sessions_are_random_and_cannot_be_forged_or_reused_after_logout(self):
        first = self.auth.login('admin','session-test-password','client')
        second = self.auth.login('admin','session-test-password','client')
        self.assertNotEqual(first,second)
        self.assertNotIn(first,self.auth.sessions)
        self.assertTrue(self.auth.authenticated(self.cookie(first)))
        self.assertFalse(self.auth.authenticated(self.cookie(first+'forged')))
        self.auth.revoke(self.cookie(first))
        self.assertFalse(self.auth.authenticated(self.cookie(first)))
        self.assertTrue(self.auth.authenticated(self.cookie(second)))
        self.assertFalse(self.auth.authenticated('broken cookie;'))

    def test_cookie_expiry_and_security_attributes(self):
        token = self.auth.login('admin','session-test-password','client')
        parsed = SimpleCookie(self.auth.cookie(token))[self.auth.cookie_name]
        self.assertTrue(parsed['httponly'])
        self.assertEqual(parsed['samesite'],'Strict')
        self.assertEqual(parsed['path'],'/')
        self.assertFalse(parsed['domain'])
        self.assertEqual(int(parsed['max-age']),SESSION_SECONDS)
        self.now += SESSION_SECONDS
        self.assertFalse(self.auth.authenticated(self.cookie(token)))
        self.assertEqual(SimpleCookie(self.auth.cookie())[self.auth.cookie_name]['max-age'],'0')
        secure = Sessions(self.credentials,'other-controller',secure=True)
        self.assertTrue(SimpleCookie(secure.cookie('token'))[secure.cookie_name]['secure'])
        self.assertNotEqual(self.auth.cookie_name,secure.cookie_name)

    def test_login_throttle_expires_and_session_store_is_bounded(self):
        for _ in range(8):
            with self.assertRaises(AuthError):
                self.auth.login('admin','wrong','limited-client')
        with self.assertRaises(AuthError) as error:
            self.auth.login('admin','session-test-password','limited-client')
        self.assertEqual(error.exception.status,429)
        self.now += 61
        token = self.auth.login('admin','session-test-password','limited-client')
        self.assertTrue(self.auth.authenticated(self.cookie(token)))
        self.auth.sessions = {str(index):self.now+60 for index in range(256)}
        self.auth.login('admin','session-test-password','another-client')
        self.assertEqual(len(self.auth.sessions),256)

    def test_password_change_revokes_sessions_and_survives_bootstrap(self):
        first = self.auth.login('admin','session-test-password','client')
        other = self.auth.login('admin','session-test-password','other-client')
        new = self.auth.change_password(self.cookie(first),'session-test-password','changed-test-password','changed-test-password','client')
        self.assertFalse(self.auth.authenticated(self.cookie(first)))
        self.assertFalse(self.auth.authenticated(self.cookie(other)))
        self.assertTrue(self.auth.authenticated(self.cookie(new)))
        with self.assertRaises(AuthError):
            self.auth.login('admin','session-test-password','client')
        self.assertTrue(self.auth.login('admin','changed-test-password','client'))
        initialize_credentials('admin','session-test-password',self.path)
        restored = Sessions(json.loads(self.path.read_text()),'test-controller')
        self.assertTrue(restored.login('admin','changed-test-password','client'))
        self.assertNotIn('changed-test-password',self.path.read_text())
        self.assertEqual(self.path.stat().st_mode & 0o777,0o600)

    def test_password_change_checks_session_current_password_confirmation_and_length(self):
        token = self.auth.login('admin','session-test-password','client')
        for current, new, confirmation in [('wrong','changed-password','changed-password'),
                ('session-test-password','short','short'),
                ('session-test-password','changed-password','different-password'),
                ('session-test-password','session-test-password','session-test-password')]:
            with self.assertRaises(AuthError):
                self.auth.change_password(self.cookie(token),current,new,confirmation,'client')
            self.assertTrue(self.auth.authenticated(self.cookie(token)))
        with self.assertRaises(AuthError) as error:
            self.auth.change_password('','session-test-password','changed-password','changed-password','client')
        self.assertEqual(error.exception.status,401)
        self.assertEqual(json.loads(self.path.read_text()),self.credentials)

    def test_failed_password_save_keeps_old_credentials_and_sessions(self):
        token = self.auth.login('admin','session-test-password','client')
        with patch('web_auth.store_credentials',side_effect=OSError('disk full')):
            with self.assertRaises(AuthError) as error:
                self.auth.change_password(self.cookie(token),'session-test-password','changed-password','changed-password','client')
        self.assertEqual(error.exception.status,503)
        self.assertTrue(self.auth.authenticated(self.cookie(token)))
        self.assertTrue(self.auth.login('admin','session-test-password','client'))
        self.assertEqual(json.loads(self.path.read_text()),self.credentials)

    def test_current_password_attempts_are_limited(self):
        token = self.auth.login('admin','session-test-password','client')
        for _ in range(8):
            with self.assertRaises(AuthError) as error:
                self.auth.change_password(self.cookie(token),'wrong','changed-password','changed-password','client')
            self.assertEqual(error.exception.status,403)
        with self.assertRaises(AuthError) as error:
            self.auth.change_password(self.cookie(token),'session-test-password','changed-password','changed-password','client')
        self.assertEqual(error.exception.status,429)
