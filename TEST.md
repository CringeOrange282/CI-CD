# CI-CD

## Локальная проверка Orchestra

Понадобятся CMake 3.20 или новее, компилятор с поддержкой C++23, Git, Python 3 и `curl`.

В примере один Python-сервер принимает запросы от Sender на `/submit` и от Notifier на `/notify`. Результат расчёта отправляется вручную, чтобы успеть добавить второго подписчика.

Если порт `8080` или `9000` уже занят, замените его на свободный во всех командах примера, включая код Python-приёмника.

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
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Event
from urllib.request import Request, urlopen

priority_started = Event()
priority_release = Event()


class Receiver(BaseHTTPRequestHandler):
    def reply(self, status):
        self.send_response(status)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self):
        if self.path != "/priority/ready":
            self.send_error(404)
            return
        self.reply(200 if priority_started.wait(timeout=10) else 504)

    def do_POST(self):
        if self.path == "/priority/release":
            priority_release.set()
            self.reply(204)
            return

        sender = {"/submit": "Sender", "/notify": "Notifier"}.get(self.path)
        if sender is None:
            self.send_error(404)
            return

        body = self.rfile.read(int(self.headers["Content-Length"]))
        payload = json.loads(body)
        print(f"\n{sender} POST {self.path}")
        print(json.dumps(payload, indent=2, ensure_ascii=False))
        if self.path == "/submit" and payload["name_process"] == "priority-gate":
            priority_started.set()
            if not priority_release.wait(timeout=25):
                self.reply(504)
                return
            result = json.dumps({
                "key": payload["dedup_key"],
                "attempt_id": payload["attempt_id"],
                "status": "Approve",
            }).encode()
            callback = Request("http://127.0.0.1:8080/result", data=result,
                               headers={"Content-Type": "application/json"})
            with urlopen(callback, timeout=5):
                pass
        self.reply(200)


with ThreadingHTTPServer(("127.0.0.1", 9000), Receiver) as server:
    print("Приёмник слушает http://127.0.0.1:9000")
    server.serve_forever()
PY
```

### 3. Подготовьте данные и запустите Orchestra

В другом терминале из корня репозитория создайте временную JSON-базу и укажите её путь при запуске Orchestra.

```bash
data_file=$(mktemp)
cat > "$data_file" <<'JSON'
[
  {
    "document": "portfolio",
    "date": "2026-11-25",
    "content": {
      "portfolio_sum": 30000
    }
  }
]
JSON

ORCHESTRA_DATA_FILE="$data_file" ./build/bin/debug/orchestra 127.0.0.1 8080 \
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
  "attempt_id": "2741915-3975313-1",
  "input_data": {
    "date": "2026-11-25",
    "doc": "portfolio",
    "portfolio_sum": 30000
  },
  "name_process": "calculate-risk"
}
```

Проверьте, что в `input_data` пришли `portfolio_sum: 30000` из файла и метаданные выбранной записи.

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

### 7. Проверьте отсутствующую запись

Запросите тот же документ за дату, которой нет в файле.

```bash
curl -sS -i http://127.0.0.1:8080/add_task \
  -H 'Content-Type: application/json' \
  --data '{"id":"demo-3","author":"alice","date":"2026-11-24","document":"portfolio","process":"calculate-risk"}'
```

Orchestra отвечает `HTTP 200` со статусом `failed`. В Python-приёмнике число запросов остаётся прежним.

```json
{
  "enqueued": false,
  "error": "No input data for document 'portfolio' on 2026-11-24",
  "request_id": "demo-3",
  "status": "failed"
}
```

### 8. Проверьте порядок приоритетов

Оставьте оба процесса запущенными и выполните блок целиком в терминале с `curl`. Первый запрос, `priority-gate`, занимает Sender. Python-приёмник удерживает ответ на этот POST, пока остальные четыре задачи ставятся в очередь. Запрос к `/priority/ready` подтверждает, что первый POST уже у приёмника, а `/priority/release` позволяет Sender закончить отправку и перейти к выбору следующей задачи.

```bash
(
  set -e

  add_priority_task() {
    curl -fsS http://127.0.0.1:8080/add_task \
      -H 'Content-Type: application/json' \
      --data "{\"id\":\"$1\",\"author\":\"alice\",\"date\":\"2026-11-25\",\"document\":\"portfolio\",\"process\":\"$1\",\"priority\":\"$2\"}"
    printf '\n'
  }

  add_priority_task priority-gate auto
  curl -fsS http://127.0.0.1:9000/priority/ready
  add_priority_task priority-low-1 auto
  add_priority_task priority-high-1 manual
  add_priority_task priority-low-2 auto
  add_priority_task priority-high-2 manual
  curl -fsS -X POST http://127.0.0.1:9000/priority/release
)
```

Все пять заявок возвращают `"status":"enqueued"`. В терминале Python-приёмника новые запросы `Sender POST /submit` идут в следующем порядке по полю `name_process`.

1. `priority-gate` — POST, который Sender уже начал.
2. `priority-high-1` — первая задача с `manual`.
3. `priority-high-2` — вторая задача с `manual`.
4. `priority-low-1` — первая ожидающая задача с `auto`.
5. `priority-low-2` — вторая ожидающая задача с `auto`.

Для `priority-gate` приёмник отправляет callback и освобождает слот. Остальные четыре задачи занимают четыре слота с настройками по умолчанию. Для повторного прогона с теми же ключами перезапустите оба процесса. После проверки остановите их сочетанием `Ctrl+C`.

## Автоматический расчёт и отмена через `service/fake_server.py`

Остановите процессы из предыдущего сценария. Используйте ту же сборку и JSON-базу из шага 3. Команды запускайте из корня репозитория.

В первом терминале установите зависимости в виртуальное окружение и запустите тестовый вычислительный сервис на порту `9001`.

```bash
python3 -m venv build/test-venv
build/test-venv/bin/python -m pip install fastapi httpx uvicorn
ORCHESTRA_CALLBACK_URL=http://127.0.0.1:8080/result \
  build/test-venv/bin/python -m uvicorn service.fake_server:app \
  --host 127.0.0.1 --port 9001
```

Во втором терминале запустите приёмник HTTP-уведомлений на порту `9000`.

```bash
python3 -u test_server.py
```

В терминале, где задана переменная `data_file` из шага 3, запустите Orchestra.

```bash
ORCHESTRA_DATA_FILE="$data_file" ./build/bin/debug/orchestra 127.0.0.1 8080 \
  http://127.0.0.1:9001 /submit \
  http://127.0.0.1:9000 /notify
```

В другом терминале отправьте заявку.

```bash
curl -fsS http://127.0.0.1:8080/add_task \
  -H 'Content-Type: application/json' \
  --data '{"author":"alice","date":"2026-11-25","document":"portfolio","process":"calculate-risk"}'
```

Сразу придёт ответ со статусом `enqueued`. Примерно через пять секунд `fake_server.py` сам отправит `POST /result` с `attempt_id`. В терминале `test_server.py` появится `Notifier POST /notify` с результатом `Approve` и `risk_score: 0.75`. Повтор той же заявки вернёт `cached`.

### Проверьте отмену по таймауту

Остановите только Orchestra и запустите его снова с таймаутом в одну секунду. Тестовый сервис и приёмник уведомлений оставьте запущенными.

```bash
load_config=$(mktemp)
cat > "$load_config" <<'JSON'
{
  "max_concurrent_requests": 1,
  "request_timeout_ms": 1000,
  "cancel_path": "/cancel"
}
JSON

ORCHESTRA_CONFIG_FILE="$load_config" ORCHESTRA_DATA_FILE="$data_file" \
  ./build/bin/debug/orchestra 127.0.0.1 8080 \
  http://127.0.0.1:9001 /submit \
  http://127.0.0.1:9000 /notify
```

Отправьте ту же заявку. Примерно через секунду в журнале Uvicorn появится успешный `POST /cancel`, а приёмник уведомлений покажет `"result":{"status":"failed","error":"Calculation timed out"}`. После уведомления проверьте, что у тестового сервиса нет активных задач.

```bash
curl -fsS http://127.0.0.1:9001/stats
```

Поле `count_tasks` должно быть равно `0`. Повтор той же заявки вернёт `enqueued` и запустит новую попытку.
