"""HTTP test client using the same login/session flow as the browser."""
from http.cookiejar import CookieJar
import json
import os
import urllib.error
import urllib.request


class Client:
    def __init__(self):
        self.base = os.environ.get('TEST_BASEURL','http://127.0.0.1:8080')
        self.jar = CookieJar()
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(self.jar))
        self.last_headers = None

    def request(self, path, body=None, auth=True, control=True, headers=None):
        supplied = dict(headers or {})
        if body is not None:
            supplied['Content-Type'] = 'application/json'
            if control:
                supplied['X-Delayer-Control'] = '1'
        req = urllib.request.Request(self.base+path, data=json.dumps(body).encode() if body is not None else None, headers=supplied)
        opener = self.opener if auth else urllib.request.build_opener()
        try:
            response = opener.open(req,timeout=35)
        except urllib.error.HTTPError as error:
            response = error
        with response:
            self.last_headers = response.headers
            data = response.read()
            return response.status, json.loads(data) if response.headers.get_content_type() == 'application/json' else data

    def login(self):
        code, data = self.request('/api/auth/login', {'username':'admin','password':os.environ['WEB_PASSWORD']})
        assert code==200 and data['authenticated'], (code,data)
