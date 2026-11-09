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
import tempfile

AUTH_FILE = '/var/lib/delayer/auth.json'
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


def store_credentials(path, credentials):
    path = Path(path)
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', dir=path.parent, delete=False) as stream:
            temporary = Path(stream.name)
            os.fchmod(stream.fileno(), 0o600)
            json.dump(credentials, stream)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if temporary:
            temporary.unlink(missing_ok=True)


def initialize_credentials(username, password, path=AUTH_FILE):
    path = Path(path)
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    path.parent.chmod(0o700)
    if path.exists():
        path.chmod(0o600)
        return
    store_credentials(path, make_credentials(username, password))


class Sessions:
    def __init__(self, credentials, namespace, secure=False, clock=None, credential_path=None):
        self._set_credentials(credentials)
        self.credential_path = Path(credential_path) if credential_path else None
        self.cookie_name = 'delayer_session_' + hashlib.sha256(namespace.encode()).hexdigest()[:12]
        self.secure = secure
        self.clock = clock or time.monotonic
        self.lock = threading.RLock()
        self.sessions = {}
        self.attempts = {}

    def _set_credentials(self, credentials):
        self.username = credentials['username']
        self.salt = bytes.fromhex(credentials['salt'])
        self.digest = bytes.fromhex(credentials['digest'])
        self.iterations = credentials['iterations']
        if len(self.salt) != 32 or len(self.digest) != 32 or self.iterations != PASSWORD_ITERATIONS:
            raise ValueError('Invalid stored password credentials')

    @classmethod
    def load(cls, namespace):
        return cls(json.loads(Path(AUTH_FILE).read_text()), namespace,
                   secure=os.environ.get('WEB_COOKIE_SECURE') == '1', credential_path=AUTH_FILE)

    def _prune(self):
        now = self.clock()
        self.sessions = {key: value for key, value in self.sessions.items() if value > now}
        self.attempts = {key: value for key, value in self.attempts.items() if value and value[-1] > now - 60}

    def _attempt(self, client):
        self._prune()
        client = str(client)[:128]
        if client not in self.attempts and len(self.attempts) >= 1024:
            self.attempts.pop(next(iter(self.attempts)))
        attempts = self.attempts.setdefault(client, deque())
        now = self.clock()
        while attempts and attempts[0] <= now - 60:
            attempts.popleft()
        if len(attempts) >= 8:
            raise AuthError(429, 'Too many password attempts. Wait a minute and try again.')
        attempts.append(now)
        return client

    def _password_matches(self, password):
        if not isinstance(password, str) or len(password) > 1024:
            return False
        try:
            given = hashlib.pbkdf2_hmac('sha256', password.encode(), self.salt, self.iterations)
            return hmac.compare_digest(given, self.digest)
        except UnicodeError:
            return False

    def _issue_session(self):
        token = secrets.token_urlsafe(32)
        self._prune()
        if len(self.sessions) >= 256:
            self.sessions.pop(next(iter(self.sessions)))
        self.sessions[hashlib.sha256(token.encode()).hexdigest()] = self.clock() + SESSION_SECONDS
        return token

    def login(self, username, password, client):
        # Verification and issuance share the password-change lock: an old password
        # cannot issue a session after a concurrent password change has completed.
        with self.lock:
            client = self._attempt(client)
            try:
                valid_name = isinstance(username, str) and len(username) <= 64 and hmac.compare_digest(username.encode(), self.username.encode())
            except UnicodeError:
                valid_name = False
            valid_password = self._password_matches(password)
            if not valid_name or not valid_password:
                raise AuthError(401, 'Incorrect username or password.')
            self.attempts.pop(client, None)
            return self._issue_session()

    def change_password(self, cookie, current_password, new_password, confirmation, client):
        with self.lock:
            if not self.authenticated(cookie):
                raise AuthError(401, 'Sign in again before changing your password.')
            client = self._attempt(client)
            if not self._password_matches(current_password):
                raise AuthError(403, 'Current password is incorrect.')
            if not isinstance(new_password, str) or not 8 <= len(new_password) <= 1024 or '\n' in new_password or '\r' in new_password:
                raise AuthError(400, 'Use a new password with 8 to 1024 characters.')
            if new_password != confirmation:
                raise AuthError(400, 'New passwords do not match.')
            if new_password == current_password:
                raise AuthError(400, 'Choose a different password from your current one.')
            try:
                credentials = make_credentials(self.username, new_password)
            except UnicodeError:
                raise AuthError(400, 'The new password contains invalid characters.')
            if not self.credential_path:
                raise AuthError(503, 'Persistent password storage is not configured.')
            try:
                store_credentials(self.credential_path, credentials)
            except OSError:
                raise AuthError(503, 'Could not save the new password. Try again.')
            self._set_credentials(credentials)
            self.sessions.clear()
            self.attempts.pop(client, None)
            return self._issue_session()

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
