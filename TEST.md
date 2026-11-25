# CI-CD

## Локальная проверка Orchestra

Понадобятся CMake 3.20 или новее, компилятор с поддержкой C++23, Git, Python 3 и `curl`.

### 1. Соберите Orchestra

Выполните из корня репозитория.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target orchestra
```

### 2. Запустите тестовый приёмник

В отдельном терминале запустите приёмник на порту `9000`. Он выводит в терминал JSON, который отправил `CalculateServiceSender`, и отвечает `HTTP 200`.

```bash
python3 -u - <<'PY'
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
PY
```

### 3. Запустите Orchestra

В другом терминале из корня репозитория укажите адрес и порт Orchestra, затем адрес и путь приёмника.

```bash
./build/bin/debug/orchestra 127.0.0.1 8080 http://127.0.0.1:9000 /submit
```

### 4. Отправьте задачу

В третьем терминале выполните запрос.

```bash
curl -sS -i http://127.0.0.1:8080/add_task \
  -H 'Content-Type: application/json' \
  --data '{"id":"demo-1","author":"demo","date":"2026-11-25","document":"portfolio","process":"calculate-risk"}'
```

Orchestra отвечает `HTTP 200`. Поле `enqueued` подтверждает постановку задачи в очередь.

```json
{"enqueued":true,"request_id":"demo-1","status":"enqueued"}
```

Терминал приёмника показывает исходящий запрос Sender.

```json
{
  "dedup_key": "[\"calculate-risk\",\"portfolio\",\"2026-11-25\"]",
  "input_data": {
    "date": "2026-11-25",
    "doc": "portfolio",
    "portfolio_sum": 1500000
  },
  "name_process": "calculate-risk"
}
```

Повторите тот же запрос, пока Orchestra запущена. Ответ будет содержать `"status":"in_progress"` и `"enqueued":false`. В терминале приёмника останется один POST.

После проверки остановите оба процесса сочетанием `Ctrl+C`.
