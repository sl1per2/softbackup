import http.server
import socketserver
import os
import threading

AGENTS_DIR = '/home/sl1per/backup/backend/agents'

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            agents = []
            for f in sorted(os.listdir(AGENTS_DIR)):
                fp = os.path.join(AGENTS_DIR, f)
                if os.path.isfile(fp):
                    agents.append((f, os.path.getsize(fp)))
            rows = ''
            for name, size in agents:
                sz = f'{size/1024/1024:.1f} MB'
                if 'linux' in name.lower():
                    icon = '\U0001F427'; platform = 'Linux'
                elif 'windows' in name.lower():
                    icon = '\U0001FA9F'; platform = 'Windows'
                else:
                    icon = '\U0001F34E'; platform = 'macOS'
                rows += f'''<tr>
                    <td style="padding:20px;border-bottom:1px solid #1a1a1a">
                        <div style="font-size:28px;margin-bottom:4px">{icon}</div>
                        <div style="font-weight:700;font-size:16px;color:#e6e6e6">{platform} Agent</div>
                        <div style="font-size:12px;color:#888;margin-top:2px">{name}</div>
                        <div style="font-size:12px;color:#555;margin-top:2px">{sz}</div>
                    </td>
                    <td style="padding:20px;border-bottom:1px solid #1a1a1a;text-align:right;vertical-align:middle">
                        <a href="/download/{name}" style="display:inline-block;padding:10px 28px;background:#00E5FF;color:#000;text-decoration:none;border-radius:8px;font-weight:700;font-size:14px">
                            \u2B07 Download
                        </a>
                    </td>
                </tr>'''
            html = f'''<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OBS Backup - Download Agent</title>
<style>
* {{ margin:0; padding:0; box-sizing:border-box; }}
body {{ background:#0a0a0a; color:#e6e6e6; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif; min-height:100vh; display:flex; align-items:center; justify-content:center; }}
.card {{ background:#111; border-radius:16px; padding:40px; max-width:640px; width:100%; box-shadow:0 8px 32px rgba(0,0,0,0.5); }}
h1 {{ font-size:28px; margin-bottom:6px; }}
h1 span {{ color:#00E5FF; }}
.sub {{ color:#888; font-size:14px; margin-bottom:32px; }}
table {{ width:100%; border-collapse:collapse; }}
a:hover {{ background:#00CCE0 !important; }}
</style>
</head>
<body>
<div class="card">
<h1><span>OBS Backup</span> Agent</h1>
<p class="sub">Choose your platform and download the agent binary.</p>
<table>{rows}</table>
</div>
</body>
</html>'''
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(html.encode())
        elif self.path.startswith('/download/'):
            fname = self.path[len('/download/'):]
            if '/' in fname or '..' in fname:
                self.send_response(400)
                self.end_headers()
                return
            fp = os.path.join(AGENTS_DIR, fname)
            if os.path.isfile(fp):
                self.send_response(200)
                self.send_header('Content-Type', 'application/octet-stream')
                self.send_header('Content-Disposition', f'attachment; filename="{fname}"')
                self.send_header('Content-Length', str(os.path.getsize(fp)))
                self.end_headers()
                with open(fp, 'rb') as f:
                    while True:
                        chunk = f.read(65536)
                        if not chunk:
                            break
                        self.wfile.write(chunk)
            else:
                self.send_response(404)
                self.end_headers()
                self.wfile.write(b'Not found')
        else:
            self.send_response(404)
            self.end_headers()

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True

if __name__ == '__main__':
    server = ThreadedHTTPServer(('0.0.0.0', 9000), Handler)
    print('Download server running on http://0.0.0.0:9000')
    server.serve_forever()
