from http.cookies import SimpleCookie
import hashlib
import json
import sys
import unittest

sys.path.insert(0, '/opt/delayer')
from web_auth import AuthError, Sessions, SESSION_SECONDS, make_credentials


class SessionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.credentials = make_credentials('admin', 'session-test-password')

    def setUp(self):
        self.now = 1000
        self.auth = Sessions(self.credentials, 'test-controller', clock=lambda: self.now)

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
