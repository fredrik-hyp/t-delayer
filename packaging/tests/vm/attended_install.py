#!/usr/bin/env python3
"""Drive APT's real attended debconf frontend inside a disposable Ubuntu VM."""
import errno
import json
import os
from pathlib import Path
import pty
import re
import select
import signal
import time

assert Path('/var/lib/delayer-vm-cloud-ready').exists(), 'Only run in the disposable test guest'
questions = [
    ('Docker network type for the delayer container:', '1'),
    ('Web UI bind address (web-bind):', '127.0.0.1'),
    ('Web UI TCP port:', '18080'),
    ('Local multicast interface IPv4 address (optional):', ''),
    ('Absolute host directory for recorded video:', '/srv/delayer-video'),
]
command = ['apt-get', '-o', 'Dpkg::Use-Pty=0', '-o', 'APT::Color=0', 'install', '-y', '/tmp/delayer.deb']
child, terminal = pty.fork()
if child == 0:
    env = dict(os.environ, DEBIAN_FRONTEND='readline', DEBIAN_PRIORITY='high',
               TERM='dumb', NEEDRESTART_MODE='a')
    os.execvpe(command[0], command, env)
output, consumed, index, status = '', 0, 0, None
ansi = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
deadline = time.monotonic() + 2400
try:
    while time.monotonic() < deadline:
        ready, _, _ = select.select([terminal], [], [], .5)
        if not ready:
            continue
        try:
            data = os.read(terminal, 65536)
        except OSError as error:
            if error.errno != errno.EIO:
                raise
            data = b''
        if not data:
            break
        text = data.decode(errors='replace')
        output += text
        print(text, end='', flush=True)
        clean = ansi.sub('', output).replace('\r', '')
        if index < len(questions):
            question, answer = questions[index]
            found = clean.find(question, consumed)
            if found >= 0:
                consumed = found + len(question)
                os.write(terminal, (answer + '\n').encode())
                index += 1
    else:
        raise AssertionError('Attended APT installation timed out')
    _, status = os.waitpid(child, 0)
    assert os.waitstatus_to_exitcode(status) == 0, 'APT failed'
    assert index == len(questions), ('Missing installation questions', questions[index:])
    transcript = ansi.sub('', output).replace('\r', '')
    for question, _ in questions:
        assert transcript.count(question) == 1, ('Question was repeated', question)
    config = json.loads(Path('/etc/t-delayer/config.json').read_text())
    assert config['network_mode'] == 'host'
    assert config['web_bind'] == '127.0.0.1' and config['web_port'] == 18080
    assert config['video_path'] == '/srv/delayer-video'
    Path('/root/delayer-attended-result.json').write_text(json.dumps({
        'frontend': 'readline', 'priority': 'high', 'terminal': True,
        'command': command, 'questions': [dict(prompt=q, answer=a) for q, a in questions],
        'all_prompts_exactly_once': True, 'saved_answers_verified': True,
    }, indent=2) + '\n')
    print('\nPASS attended APT installation: all prompts exactly once and saved answers verified', flush=True)
finally:
    os.close(terminal)
    if status is None:
        os.kill(child, signal.SIGKILL)
        os.waitpid(child, 0)
