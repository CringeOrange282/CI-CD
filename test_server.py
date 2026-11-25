import json
from http.server import BaseHTTPRequestHandler, HTTPServer


class Receiver(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/submit":
            self.send_error(404)
            return

        body = self.rfile.read(int(self.headers["Content-Length"]))
        print(json.dumps(json.loads(body), indent=2, ensure_ascii=False))
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()


with HTTPServer(("127.0.0.1", 9000), Receiver) as server:
    print("Приёмник слушает http://127.0.0.1:9000/submit")
    server.serve_forever()
