import http.server
import json
import subprocess
import sys
import threading
import time

mode = "ok"
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Length", "2")
        self.end_headers()
        self.wfile.write(b"{}")
    def do_POST(self):
        data = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        assert self.path == "/v1/chat/completions"
        assert data["stream"] and data["max_tokens"] == 128
        assert not data["chat_template_kwargs"]["enable_thinking"]
        assert data["messages"][0]["role"] == "system"
        if mode == "error":
            payload = b'{"error":"unavailable"}'
            self.send_response(503)
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Connection", "close")
        self.end_headers()
        if mode == "malformed":
            self.wfile.write(b"data: invalid-json\n\n")
            self.wfile.flush()
            return
        for fragment in ["Привет, ", "путник!"]:
            encoded = ("data: " + json.dumps({"choices": [{"delta": {"content": fragment}}]}, ensure_ascii=False) + "\r\n\r\n").encode()
            # Split SSE and UTF-8 across network chunks.
            for pos in range(0, len(encoded), 7):
                self.wfile.write(encoded[pos:pos+7])
                self.wfile.flush()
            time.sleep(.02)
        self.wfile.write(b'data: {"usage":{"prompt_tokens":12,"completion_tokens":5},"choices":[]}\n\ndata: [DONE]\n\n')
        self.wfile.flush()

server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
try:
    for mode in ["ok", "error", "malformed"]:
        result = subprocess.run([sys.argv[1], str(server.server_port)], text=True, capture_output=True, timeout=12)
        assert result.returncode == 0, result.stderr
        reply = json.loads(result.stdout)
        assert reply["npc"] == 3
        if mode == "ok":
            assert reply["text"] == "Привет, путник!" and not reply["error"]
            assert reply["tokens"] == 5 and reply["first_token_ms"] >= 0
        elif mode == "error":
            assert reply["error"] == "http_503"
        else:
            assert reply["error"] == "invalid_stream"
finally:
    server.shutdown()
    server.server_close()
print("Streaming UTF-8, request limits, HTTP failures and malformed SSE checks passed")
