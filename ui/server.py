#!/usr/bin/env python3
"""Local web UI server (standard library only).  Run:  python3 ui/server.py   ->  http://localhost:8000"""
import json, os, re, subprocess, threading
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ".exe" if os.name == "nt" else ""
GEN = ROOT / "bin" / ("traffic_generator" + EXE)
ANALYZER = ROOT / "bin" / ("network_traffic_analysis" + EXE)
DATA = ROOT / "data"
RUN_LOCK = threading.Lock()          # one benchmark at a time keeps timings clean
MAX_RECORDS = 5_000_000


def dataset_for(records):
    path = DATA / f"traffic_{records}.csv"
    if not path.exists():
        subprocess.run([str(GEN), str(records), str(path)], check=True, capture_output=True, timeout=600)
    return path


class Handler(BaseHTTPRequestHandler):
    def _send(self, code, body, ctype="application/json"):
        data = body if isinstance(body, bytes) else json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _body(self):
        return self.rfile.read(int(self.headers.get("Content-Length", 0)))

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            return self._send(200, (ROOT / "ui" / "index.html").read_bytes(), "text/html; charset=utf-8")
        if self.path == "/api/status":
            return self._send(200, {"built": ANALYZER.exists() and GEN.exists(), "cpus": os.cpu_count(),
                                    "uploaded": (DATA / "uploaded.csv").exists()})
        self._send(404, {"error": "not found"})

    def do_POST(self):
        if not (ANALYZER.exists() and GEN.exists()):
            return self._send(500, {"error": "C++ programs are not built. Run `make` in the project folder first."})
        try:
            if self.path == "/api/upload":
                DATA.mkdir(exist_ok=True)
                (DATA / "uploaded.csv").write_bytes(self._body())
                return self._send(200, {"ok": True})
            if self.path == "/api/analyze":
                req = json.loads(self._body() or b"{}")
                threads = [int(t) for t in req.get("threads", [1, 2, 4, 8])][:12]
                repeats = max(1, min(int(req.get("repeats", 3)), 20))
                with RUN_LOCK:
                    if req.get("file") == "uploaded":
                        path = DATA / "uploaded.csv"
                        if not path.exists():
                            return self._send(400, {"error": "No uploaded file yet."})
                    else:
                        n = int(req.get("records", 100000))
                        if not 1 <= n <= MAX_RECORDS:
                            return self._send(400, {"error": f"records must be 1..{MAX_RECORDS}"})
                        path = dataset_for(n)
                    out = subprocess.run([str(ANALYZER), str(path), "--json", "--repeats", str(repeats),
                                          "--threads", ",".join(map(str, threads))],
                                         capture_output=True, text=True, timeout=900)
                if out.returncode != 0:
                    return self._send(400, {"error": out.stderr.strip() or "Analysis failed."})
                return self._send(200, out.stdout.encode())
            self._send(404, {"error": "not found"})
        except (ValueError, json.JSONDecodeError):
            self._send(400, {"error": "Invalid request."})
        except subprocess.SubprocessError as e:
            self._send(500, {"error": f"Run failed: {e}"})

    def log_message(self, fmt, *args):
        pass


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8000))
    print(f"Network Traffic Analysis UI -> http://localhost:{port}   (Ctrl+C to stop)")
    ThreadingHTTPServer((os.environ.get("HOST", "127.0.0.1"), port), Handler).serve_forever()
