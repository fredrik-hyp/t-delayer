#!/usr/bin/env python3
"""Exercise real attended debconf/readline prompts in the disposable package test.

Docker and systemd are provided by debian.sh's test doubles. This script drives
the real terminal/frontend, config script and package maintainer hooks.
"""
import argparse
import errno
import json
import os
from pathlib import Path
import pty
import re
import select
import signal
import time

assert os.environ.get('DELAYER_PACKAGE_TEST') == 'isolated', 'Use the isolated package test only'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--initial', action='store_true')
args = parser.parse_args()
PROMPT = 'Web UI bind address (web-bind):'
CONFIG = Path('/etc/t-delayer/config.json')
TRANSCRIPT = Path(os.environ.get('DELAYER_ATTENDED_TRANSCRIPT', '/tmp/attended-installer.log'))
ANSI = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')


def drive(command, answers, priority='high'):
    child, terminal = pty.fork()
    if child == 0:
        env = dict(os.environ, DEBIAN_FRONTEND='readline', DEBIAN_PRIORITY=priority, TERM='dumb')
        os.execvpe(command[0], command, env)
    output, consumed, index = '', 0, 0
    deadline = time.monotonic() + 180
    status = None
    try:
        while time.monotonic() < deadline:
            ready, _, _ = select.select([terminal], [], [], .2)
            if ready:
                try:
                    data = os.read(terminal, 65536)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    data = b''
                if not data:
                    break
                output += data.decode(errors='replace')
                clean = ANSI.sub('', output).replace('\r', '')
                if index < len(answers):
                    question, reply = answers[index]
                    found = clean.find(question, consumed)
                    if found >= 0:
                        consumed = found + len(question)
                        os.write(terminal, (reply + '\n').encode())
                        index += 1
        else:
            raise AssertionError('Attended installer timed out\n' + output)
        _, status = os.waitpid(child, 0)
        assert os.waitstatus_to_exitcode(status) == 0, output
        assert index == len(answers), ('Missing expected prompts', answers[index:], output)
        clean = ANSI.sub('', output).replace('\r', '')
        with TRANSCRIPT.open('a') as log:
            log.write('\nCOMMAND: ' + ' '.join(command) + '\n' + clean)
        return clean
    finally:
        os.close(terminal)
        if status is None:
            os.kill(child, signal.SIGKILL)
            os.waitpid(child, 0)


def full_answers(mode, bind, port):
    return [('Docker network type for the delayer container:', mode), (PROMPT, bind),
            ('Web UI TCP port:', str(port)), ('Local multicast interface IPv4 address (optional):', ''),
            ('Absolute host directory for recorded video:', '/var/lib/t-delayer/video')]


def configuration():
    return json.loads(CONFIG.read_text())


if args.initial:
    transcript = drive(['dpkg', '--force-depends', '--configure', 't-delayer-docker'],
                       full_answers('2', '127.0.0.1', 18080))
    assert transcript.count(PROMPT) == 1, transcript
    assert configuration()['web_bind'] == '127.0.0.1'
    print('PASS fresh attended package installation displays web-bind exactly once')
else:
    before = CONFIG.read_bytes()
    # APT can invoke config again after preconfiguration. Seen answers must not
    # be reset or the same web-bind question would be asked twice.
    transcript = drive(['debconf', '--frontend=readline', '--priority=high',
                        '/var/lib/dpkg/info/t-delayer-docker.config', 'configure', '4.1.2'], [])
    assert PROMPT not in transcript and CONFIG.read_bytes() == before
    transcript = drive(['dpkg-reconfigure', '-f', 'readline', '-p', 'high', 't-delayer-docker'],
                       full_answers('2', '0.0.0.0', 18082))
    assert transcript.count(PROMPT) == 1
    assert configuration()['web_bind'] == '0.0.0.0'
    assert '0.0.0.0:18082:8080' in json.loads(Path('/tmp/fake-container.args').read_text())
    transcript = drive(['dpkg-reconfigure', '-f', 'readline', '-p', 'critical', 't-delayer-docker'],
                       [(PROMPT, '127.0.0.2')], priority='critical')
    assert transcript.count(PROMPT) == 1 and 'Docker network type for the delayer container:' not in transcript
    assert configuration()['web_bind'] == '127.0.0.2'
    assert '127.0.0.2:18082:8080' in json.loads(Path('/tmp/fake-container.args').read_text())
    transcript = drive(['dpkg-reconfigure', '-f', 'readline', '-p', 'high', 't-delayer-docker'],
                       full_answers('1', '127.0.0.2', 18081))
    assert transcript.count(PROMPT) == 1
    assert configuration()['web_bind'] == '127.0.0.2'
    public = json.loads(Path('/tmp/fake-web-env.json').read_text())
    assert public['WEB_BIND'] == '127.0.0.2' and public['WEB_PORT'] == '18081'
    print('PASS attended loopback/all/custom IPv4, critical priority, host/bridge propagation and config idempotence')
