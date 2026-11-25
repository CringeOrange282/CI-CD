from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import time
import random

class MockExternalService(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        raw_data = self.rfile.read(content_length).decode('utf-8')
        req = json.loads(raw_data)

        author = req.get("author", "unknown")
        process = req.get("process", "unknown")
        document = req.get("document", "unknown")
        date = req.get("date", "unknown")

        print(f"[Service] Received: process='{process}', document='{document}', date='{date}' (from {author})")

        sleep_sec = round(random.uniform(2.0, 4.0), 2)
        time.sleep(sleep_sec)

        response = {
            "author": author,
            "date": date,
            "document": document,
            "process": process,
            "status": "DONE",
            "calc_time_sec": sleep_sec,
            "result_value": round(random.uniform(1000.0, 999999.0), 2)
        }

        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.end_headers()
        self.wfile.write(json.dumps(response, ensure_ascii=False).encode('utf-8'))

        print(f"[Service] Completed '{process} - {document}' in {sleep_sec}s. Response sent.")

if __name__ == "__main__":
    server = HTTPServer(('localhost', 8080), MockExternalService)
    print("Mock service running on http://localhost:8080 ...")
    server.serve_forever()
