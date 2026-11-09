"""Exercise password changes over nginx and verify persistent credentials."""
import json
import os
from pathlib import Path
import sys

from auth_client import Client

password = 'rotated-test-password'
client = Client()
body = {'current_password':os.environ['WEB_PASSWORD'],'new_password':password,'confirm_password':password}
if sys.argv[1] == 'change':
    assert client.request('/api/auth/password',body,auth=False)[0] == 401
    client.login()
    other = Client()
    other.login()
    old_cookie = '; '.join(item.name+'='+item.value for item in client.jar)
    assert client.request('/api/auth/password',body,headers={'Origin':'https://other.example'})[0] == 403
    assert client.request('/api/auth/password',dict(body,current_password='wrong'))[0] == 403
    assert client.request('/api/status')[0] == 200
    assert client.request('/api/auth/password',dict(body,confirm_password='different-password'))[0] == 400
    assert client.request('/api/auth/password',dict(body,new_password='short',confirm_password='short'))[0] == 400
    assert client.request('/api/auth/password',{})[0] == 400
    code, result = client.request('/api/auth/password',body)
    assert code == 200 and result['changed'] and result['authenticated'], (code,result)
    assert client.request('/api/status')[0] == 200
    assert other.request('/api/status')[0] == 401
    assert client.request('/api/status',auth=False,headers={'Cookie':old_cookie})[0] == 401
    Path('/tmp/changed-password-cookie').write_text('; '.join(item.name+'='+item.value for item in client.jar))
    print('PASS password change requires current password/session, validates confirmation, rotates cookie and revokes all old sessions')
else:
    cookie_file = Path('/tmp/changed-password-cookie')
    if cookie_file.exists():
        assert client.request('/api/status',auth=False,headers={'Cookie':cookie_file.read_text()})[0] == 401
assert client.request('/api/auth/login',{'username':'admin','password':os.environ['WEB_PASSWORD']},auth=False)[0] == 401
assert client.request('/api/auth/login',{'username':'admin','password':password})[0] == 200
stored = Path('/var/lib/delayer/auth.json')
assert password not in stored.read_text()
assert stored.stat().st_mode & 0o777 == 0o600
print('PASS new password works, bootstrap password is rejected and persistent storage contains only a protected hash')
