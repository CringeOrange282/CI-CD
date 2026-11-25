from fastapi import FastAPI,Response
from pydantic import BaseModel
from datetime import date
import time
import asyncio
import httpx

app = FastAPI()

start_time = time.time()

class Transaction(BaseModel):
    amount:float
    type:str
    currency:str

class UserInfo(BaseModel):
    client_id: str
    full_name:str
    inn:str
    phone_number:str
    email:str | None = None
    address:str
    birth_date: date
    client_type:str
    credit_history_score:int



class Request(BaseModel):
    user:UserInfo
    operations:Transaction

callback_url = "localhost:8080/result"#URL Orhestrator

active_requests = {}
complete_requests = {}
@app.post("/requests/{dedup_key}")
async def add_request(request:Request,dedup_key:str):
    active_requests[dedup_key] = {
        "start_time_process": time.time(),
        "status": "processing"
    }
    await asyncio.sleep(5)
    risk_score = request.operations.amount/(request.operations.amount + 10_000.00)
    if(risk_score < 0.5):status = "Disapprove"
    else:status = "Approve"
    result = {
        "risk_score": risk_score,"status":status,"key":dedup_key
    }
    complete_requests[dedup_key] = {
        "result" : result,
        "time_complete":time.time() - active_requests.get(dedup_key).get("start_time_process")
    }
    active_requests.pop(dedup_key,None)
    async with httpx.AsyncClient() as client:
        await client.post(request.callback_url,json=result)
    return Response(status_code=202)

@app.get("/stats")
async def stats():
    return {
        "count_tasks" : len(active_requests),
        "tasks" : list(active_requests.keys()),
        "count_complete_tasks":len(complete_requests),
        "complete_tasks": list(complete_requests.keys())[-10:]
    }