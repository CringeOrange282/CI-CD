# CI-CD

[Русская версия](README.ru.md)

## Run Orchestra

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target orchestra
./build/bin/debug/orchestra
```

Orchestra listens on `127.0.0.1:8080` by default. Set `ORCHESTRA_HOST` and
`ORCHESTRA_PORT` to change the bind address. Set `ORCHESTRA_DATA_URL` to the
origin of your information service. It defaults to `http://127.0.0.1:8081`.
That service must expose `GET /information` with query parameters
`information_id` and `information_from_date`. Return `200` with JSON data
for a match or `404` if the data is missing.

## Submit an operation

```sh
curl http://127.0.0.1:8080/requests \
  -H 'Content-Type: application/json' \
  -d '{"user":"alice","operation":"operation_type1","information_id":"example","information_from_date":"2026-11-24"}'
```

The four fields must be non-empty strings. Matching requests share an operation
ID when `operation`, `information_id`, and `information_from_date` match.
Successful submissions return `202` with the operation ID while the job is
pending, or `200` with the cached result once it is complete. An information
lookup returns `404` for missing data, `504` when the HTTP client reports a
timeout, or `502` for other upstream failures. Poll
`GET /operations/<operation_id>` for the status and result.

## Worker API

1. Call `POST /queue/next` to get the next job. The response is `200` with
   `operation_id`, `operation`, and `process_data`, or `204` when the queue
   is empty. `process_data` is the JSON returned by the information service.
   The same job is returned until its result is submitted.
2. Submit `{"result": ...}` with `PUT /operations/<operation_id>/result`.
   A successful submission caches the result and advances the queue. Retrying
   the same result succeeds. Submitting a different result for a completed
   operation returns `409`.

The worker API serves one active job at a time. The queue and result cache
are held in memory and reset when Orchestra restarts. The HTTP endpoints
are unauthenticated.
