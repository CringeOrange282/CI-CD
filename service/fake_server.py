import asyncio
import os
import time

import httpx
from fastapi import FastAPI
from pydantic import BaseModel, FiniteFloat

app = FastAPI()

callback_url = os.environ.get(
    "ORCHESTRA_CALLBACK_URL", "http://127.0.0.1:8080/result"
)
active_requests = {}
complete_requests = {}
pending_cancellations = set()


class InputData(BaseModel):
    doc: str
    date: str
    portfolio_sum: FiniteFloat


class CalculateRequest(BaseModel):
    dedup_key: str
    attempt_id: str | None = None
    input_data: InputData
    name_process: str


class CancelRequest(BaseModel):
    dedup_key: str
    attempt_id: str


async def calculate(request: CalculateRequest, started_at: float):
    key = request.dedup_key
    attempt = (key, request.attempt_id)
    await asyncio.sleep(5)

    amount = request.input_data.portfolio_sum
    if amount == -10_000:
        result = {"key": key, "status": "failed", "error": "Undefined risk score"}
    else:
        risk_score = amount / (amount + 10_000)
        status = "Disapprove" if risk_score < 0.5 else "Approve"
        result = {"key": key, "status": status, "risk_score": risk_score}

    active_requests.pop(attempt)
    if request.attempt_id is not None:
        result["attempt_id"] = request.attempt_id
    complete_requests[attempt] = {
        "result": result,
        "time_complete": time.monotonic() - started_at,
    }
    async with httpx.AsyncClient() as client:
        response = await client.post(callback_url, json=result)
        response.raise_for_status()


@app.post("/submit")
async def add_request(request: CalculateRequest):
    attempt = (request.dedup_key, request.attempt_id)
    if attempt in pending_cancellations:
        pending_cancellations.remove(attempt)
        return {"status": "cancelled"}
    if attempt not in active_requests:
        active_requests[attempt] = asyncio.create_task(
            calculate(request, time.monotonic())
        )
    return {"status": "accepted"}


@app.post("/cancel")
async def cancel_request(request: CancelRequest):
    attempt = (request.dedup_key, request.attempt_id)
    task = active_requests.pop(attempt, None)
    if task is not None:
        task.cancel()
    elif attempt not in complete_requests:
        # Отмена может прийти по другому соединению раньше запроса на запуск.
        pending_cancellations.add(attempt)
    return {"status": "cancelled"}


@app.get("/stats")
async def stats():
    return {
        "count_tasks": len(active_requests),
        "tasks": [key for key, _ in active_requests],
        "count_complete_tasks": len(complete_requests),
        "complete_tasks": [key for key, _ in list(complete_requests)[-10:]],
    }
