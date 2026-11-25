import asyncio
import os
import time

import httpx
from fastapi import BackgroundTasks, FastAPI
from pydantic import BaseModel, FiniteFloat

app = FastAPI()

callback_url = os.environ.get(
    "ORCHESTRA_CALLBACK_URL", "http://127.0.0.1:8080/result"
)
active_requests = {}
complete_requests = {}


class InputData(BaseModel):
    doc: str
    date: str
    portfolio_sum: FiniteFloat


class CalculateRequest(BaseModel):
    dedup_key: str
    input_data: InputData
    name_process: str


async def calculate(request: CalculateRequest):
    key = request.dedup_key
    await asyncio.sleep(5)

    amount = request.input_data.portfolio_sum
    if amount == -10_000:
        result = {"key": key, "status": "failed", "error": "Undefined risk score"}
    else:
        risk_score = amount / (amount + 10_000)
        status = "Disapprove" if risk_score < 0.5 else "Approve"
        result = {"key": key, "status": status, "risk_score": risk_score}

    task = active_requests.pop(key)
    complete_requests[key] = {
        "result": result,
        "time_complete": time.monotonic() - task["start_time_process"],
    }
    async with httpx.AsyncClient() as client:
        response = await client.post(callback_url, json=result)
        response.raise_for_status()


@app.post("/submit")
async def add_request(request: CalculateRequest, tasks: BackgroundTasks):
    key = request.dedup_key
    if key not in active_requests:
        active_requests[key] = {
            "start_time_process": time.monotonic(),
            "status": "processing",
        }
        tasks.add_task(calculate, request)
    return {"status": "accepted"}


@app.get("/stats")
async def stats():
    return {
        "count_tasks": len(active_requests),
        "tasks": list(active_requests),
        "count_complete_tasks": len(complete_requests),
        "complete_tasks": list(complete_requests)[-10:],
    }
