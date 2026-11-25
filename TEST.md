# CI-CD

## Локальная проверка Orchestra

Понадобятся CMake 3.20 или новее, компилятор с поддержкой C++23, Git, Python 3 и `curl`.

В примере один Python-сервер принимает запросы от Sender на `/submit` и от Notifier на `/notify`. Результат расчёта отправляется вручную, чтобы успеть добавить второго подписчика.

### 1. Соберите Orchestra

Выполните из корня репозитория.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --target orchestra
```

### 2. Запустите тестовый Python-приёмник

В отдельном терминале запустите приёмник на порту `9000`. Для каждого POST он печатает имя отправителя, путь и тело JSON, затем отвечает `HTTP 200`.

```bash
python3 -u - <<'PY'
import json
from http.server import BaseHTTPRequestHandler, HTTPServer


class Receiver(BaseHTTPRequestHandler):
    def do_POST(self):
        sender = {"/submit": "Sender", "/notify": "Notifier"}.get(self.path)
        if sender is None:
            self.send_error(404)
            return

        body = self.rfile.read(int(self.headers["Content-Length"]))
        print(f"\n{sender} POST {self.path}")
        print(json.dumps(json.loads(body), indent=2, ensure_ascii=False))
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()


with HTTPServer(("127.0.0.1", 9000), Receiver) as server:
    print("Приёмник слушает http://127.0.0.1:9000")
    server.serve_forever()
PY
```

### 3. Запустите Orchestra

В другом терминале из корня репозитория запустите Orchestra. После адреса и порта Orchestra укажите адрес и путь для Sender, затем адрес и путь для Notifier.

```bash
./build/bin/debug/orchestra 127.0.0.1 8080 \
  http://127.0.0.1:9000 /submit \
  http://127.0.0.1:9000 /notify
```

### 4. Отправьте задачу

В третьем терминале выполните запрос.

```bash
curl -sS -i http://127.0.0.1:8080/add_task \
  -H 'Content-Type: application/json' \
  --data '{"id":"demo-1","author":"alice","date":"2026-11-25","document":"portfolio","process":"calculate-risk"}'
```

Orchestra отвечает `HTTP 200`. Поле `enqueued` подтверждает постановку задачи в очередь.

```json
{"enqueued":true,"request_id":"demo-1","status":"enqueued"}
```

В терминале Python-приёмника под строкой `Sender POST /submit` появится запрос.

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

### 5. Добавьте второго подписчика

Отправьте запрос от `bob` с теми же процессом, документом и датой.

```bash
curl -sS -i http://127.0.0.1:8080/add_task \
  -H 'Content-Type: application/json' \
  --data '{"id":"demo-2","author":"bob","date":"2026-11-25","document":"portfolio","process":"calculate-risk"}'
```

Ответ содержит `"status":"in_progress"` и `"enqueued":false`. В терминале приёмника по-прежнему виден один запрос `Sender POST /submit`.

### 6. Передайте результат и проверьте уведомление

Отправьте результат в `POST /result`. Поле `key` содержит тот же ключ, который Sender передал в поле `dedup_key`.

```bash
curl -sS -i http://127.0.0.1:8080/result \
  -H 'Content-Type: application/json' \
  --data '{"key":"[\"calculate-risk\",\"portfolio\",\"2026-11-25\"]","status":"Approve","risk_score":0.75}'
```

Orchestra отвечает `HTTP 200`. В терминале Python-приёмника под строкой `Notifier POST /notify` появится одно уведомление для обоих подписчиков.

```json
{
  "список получателей": ["alice", "bob"],
  "document": "portfolio",
  "date": "2026-11-25",
  "operation": "calculate-risk",
  "result": {
    "key": "[\"calculate-risk\",\"portfolio\",\"2026-11-25\"]",
    "status": "Approve",
    "risk_score": 0.75
  }
}
```

Повторите первый `POST /add_task`. Orchestra вернёт `"status":"cached"` и готовый расчёт в поле `result`. В терминале Python-приёмника останется по одному POST на каждый путь.

После проверки остановите оба процесса сочетанием `Ctrl+C`.
