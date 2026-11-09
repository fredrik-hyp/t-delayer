"""Password verification and bounded, revocable browser sessions for the web UI."""
from collections import deque
import hashlib
import hmac
from http.cookies import CookieError, SimpleCookie
import json
import os
from pathlib import Path
import secrets
import threading
import time

AUTH_FILE = '/run/delayer/auth.json'
SESSION_SECONDS = 8 * 60 * 60
PASSWORD_ITERATIONS = 600000


class AuthError(Exception):
    def __init__(self, status, message):
        self.status, self.message = status, message


def make_credentials(username, password):
    salt = secrets.token_bytes(32)
    digest = hashlib.pbkdf2_hmac('sha256', password.encode(), salt, PASSWORD_ITERATIONS)
    return {'username': username, 'salt': salt.hex(), 'digest': digest.hex(),
            'iterations': PASSWORD_ITERATIONS}


class Sessions:
    def __init__(self, credentials, namespace, secure=False, clock=None):
        self.username = credentials['username']
        self.salt = bytes.fromhex(credentials['salt'])
        self.digest = bytes.fromhex(credentials['digest'])
        self.iterations = credentials['iterations']
        self.cookie_name = 'delayer_session_' + hashlib.sha256(namespace.encode()).hexdigest()[:12]
        self.secure = secure
        self.clock = clock or time.monotonic
        self.lock = threading.RLock()
        self.sessions = {}
        self.attempts = {}

    @classmethod
    def load(cls, namespace):
        return cls(json.loads(Path(AUTH_FILE).read_text()), namespace,
                   secure=os.environ.get('WEB_COOKIE_SECURE') == '1')

    def _prune(self):
        now = self.clock()
        self.sessions = {key: value for key, value in self.sessions.items() if value > now}
        self.attempts = {key: value for key, value in self.attempts.items() if value and value[-1] > now - 60}

    def login(self, username, password, client):
        with self.lock:
            self._prune()
            client = str(client)[:128]
            if client not in self.attempts and len(self.attempts) >= 1024:
                self.attempts.pop(next(iter(self.attempts)))
            attempts = self.attempts.setdefault(client, deque())
            now = self.clock()
            while attempts and attempts[0] <= now - 60:
                attempts.popleft()
            if len(attempts) >= 8:
                raise AuthError(429, 'Too many sign-in attempts. Wait a minute and try again.')
            attempts.append(now)
        if not isinstance(username, str) or not isinstance(password, str) or len(username) > 64 or len(password) > 1024:
            raise AuthError(401, 'Incorrect username or password.')
        try:
            given = hashlib.pbkdf2_hmac('sha256', password.encode(), self.salt, self.iterations)
            valid_name = hmac.compare_digest(username.encode(), self.username.encode())
        except UnicodeError:
            raise AuthError(401, 'Incorrect username or password.')
        if not hmac.compare_digest(given, self.digest) or not valid_name:
            raise AuthError(401, 'Incorrect username or password.')
        token = secrets.token_urlsafe(32)
        with self.lock:
            self._prune()
            self.attempts.pop(client, None)
            if len(self.sessions) >= 256:
                self.sessions.pop(next(iter(self.sessions)))
            self.sessions[hashlib.sha256(token.encode()).hexdigest()] = self.clock() + SESSION_SECONDS
        return token

    def token_from_cookie(self, header):
        if not header or len(header) > 8192:
            return None
        cookie = SimpleCookie()
        try:
            cookie.load(header)
            value = cookie.get(self.cookie_name)
            token = value.value if value else None
            if token and len(token) <= 128 and token.isascii():
                return token
        except CookieError:
            pass
        return None

    def authenticated(self, header):
        token = self.token_from_cookie(header)
        if not token:
            return False
        with self.lock:
            self._prune()
            return hashlib.sha256(token.encode()).hexdigest() in self.sessions

    def revoke(self, header):
        token = self.token_from_cookie(header)
        if token:
            with self.lock:
                self.sessions.pop(hashlib.sha256(token.encode()).hexdigest(), None)

    def cookie(self, token=None):
        cookie = SimpleCookie()
        cookie[self.cookie_name] = token or ''
        value = cookie[self.cookie_name]
        value['path'] = '/'
        value['httponly'] = True
        value['samesite'] = 'Strict'
        value['max-age'] = SESSION_SECONDS if token else 0
        if not token:
            value['expires'] = 'Thu, 01 Jan 1970 00:00:00 GMT'
        if self.secure:
            value['secure'] = True
        return value.OutputString()
