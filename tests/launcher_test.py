"""Exercise launcher readiness, ownership, failures and signal cleanup."""
import http.server
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / 'scripts/run.sh'


def free_port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def wait_file(path, process):
    deadline = time.monotonic() + 12
    while not path.exists():
        assert process.poll() is None, 'Launcher exited before starting game'
        assert time.monotonic() < deadline, 'Game startup timed out'
        time.sleep(.05)


with tempfile.TemporaryDirectory(prefix='frontier launcher ') as directory:
    temp = Path(directory)
    server = temp / 'fake server'
    game = temp / 'fake game'
    model = temp / 'fake model.gguf'
    model.touch()
    server.write_text('''#!/usr/bin/env python3
import http.server, json, os, pathlib, signal, sys, time
pathlib.Path(os.environ['SERVER_PID']).write_text(str(os.getpid()))
def stop(*args):
    pathlib.Path(os.environ['SERVER_STOPPED']).touch()
    sys.exit(0)
signal.signal(signal.SIGTERM, stop)
mode = os.environ.get('SERVER_MODE', 'ready')
if mode == 'exit': sys.exit(9)
start = time.monotonic()
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args): pass
    def do_GET(self):
        ready = mode != 'timeout' and time.monotonic() - start > .3
        data = json.dumps({'data':[{'id':'forest-npc'}]}).encode()
        self.send_response(200 if ready else 503)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)
port = int(sys.argv[sys.argv.index('--port')+1])
http.server.HTTPServer(('127.0.0.1', port), Handler).serve_forever()
''')
    game.write_text('''#!/usr/bin/env python3
import json, os, pathlib, sys, time
pathlib.Path(os.environ['GAME_RECORD']).write_text(json.dumps({'pid':os.getpid(),'args':sys.argv[1:],'cwd':os.getcwd()}))
time.sleep(float(os.environ.get('GAME_SECONDS', '.3')))
sys.exit(int(os.environ.get('GAME_EXIT', '0')))
''')
    server.chmod(0o755)
    game.chmod(0o755)
    base = dict(os.environ, GAME_BINARY=str(game), LLAMA_SERVER=str(server))

    def environment(name, **extra):
        return dict(base, LLM_PORT=str(free_port()), SERVER_PID=str(temp / (name + '.pid')),
                    SERVER_STOPPED=str(temp / (name + '.stopped')),
                    GAME_RECORD=str(temp / (name + '.game')), **extra)

    def run(env, *args):
        return subprocess.run([str(LAUNCHER), '--model', str(model), *args],
                              cwd=temp, env=env, capture_output=True, text=True, timeout=20)

    env = environment('normal', GAME_EXIT='7')
    result = run(env, '--', '--save', 'a world.json')
    assert result.returncode == 7, result.stderr
    record = json.loads(Path(env['GAME_RECORD']).read_text())
    assert record['args'] == ['--llm-port', env['LLM_PORT'], '--save', 'a world.json']
    assert record['cwd'] == str(ROOT)
    assert Path(env['SERVER_STOPPED']).exists(), 'Owned model leaked after game exit'

    # Game-only sessions work without any model files or server executable.
    env = environment('game-only', GAME_EXIT='4')
    env['LLAMA_SERVER'] = str(temp / 'missing server')
    result = subprocess.run([str(LAUNCHER), '--no-model', '--model', str(temp / 'missing.gguf'),
                             '--', '--width', '1280'], cwd=temp, env=env,
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 4, result.stderr
    record = json.loads(Path(env['GAME_RECORD']).read_text())
    assert record['args'] == ['--width', '1280']
    assert record['cwd'] == str(ROOT)
    assert not Path(env['SERVER_PID']).exists(), 'Game-only launch started a server'

    for name, mode in [('early', 'exit'), ('timeout', 'timeout')]:
        env = environment(name, SERVER_MODE=mode)
        result = run(env, '--startup-timeout', '1')
        assert result.returncode == 1, result.stderr
        assert not Path(env['GAME_RECORD']).exists()
        if mode == 'timeout':
            assert Path(env['SERVER_STOPPED']).exists(), 'Timed-out model leaked'

    class Existing(http.server.BaseHTTPRequestHandler):
        alias = 'forest-npc'
        def log_message(self, *args):
            pass
        def do_GET(self):
            body = json.dumps({'data': [{'id': self.alias}]}).encode()
            self.send_response(200)
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    existing = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Existing)
    threading.Thread(target=existing.serve_forever, daemon=True).start()
    try:
        env = environment('reuse')
        env['LLM_PORT'] = str(existing.server_port)
        result = run(env)
        assert result.returncode == 0, result.stderr
        assert 'Reusing ready' in result.stdout
        assert not Path(env['SERVER_PID']).exists()
        Existing.alias = 'unrelated'
        env['GAME_RECORD'] = str(temp / 'collision.game')
        result = run(env)
        assert result.returncode == 1 and 'already serves HTTP' in result.stderr
        assert not Path(env['GAME_RECORD']).exists()
    finally:
        existing.shutdown()
        existing.server_close()

    env = environment('signal', GAME_SECONDS='60')
    process = subprocess.Popen([str(LAUNCHER), '--model', str(model)], cwd=temp, env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        wait_file(Path(env['GAME_RECORD']), process)
        process.send_signal(signal.SIGTERM)
        output, errors = process.communicate(timeout=15)
        assert process.returncode == 143, errors
        assert Path(env['SERVER_STOPPED']).exists(), 'Owned model leaked on SIGTERM'
        pid = json.loads(Path(env['GAME_RECORD']).read_text())['pid']
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            pass
        else:
            raise AssertionError('Game leaked on SIGTERM')
    finally:
        if process.poll() is None:
            process.terminate()
            process.communicate(timeout=15)

    for options in [dict(LLM_PORT='0'), dict(LLM_STARTUP_TIMEOUT='0')]:
        env = environment('invalid')
        env.update(options)
        assert run(env).returncode == 2

print('Launcher readiness, argument forwarding, exit status, reuse, collision, timeout and signal cleanup passed')
