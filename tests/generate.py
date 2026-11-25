import json
import random
from datetime import datetime, timedelta

PROCESSES = [
    "CreditRisk_Calc",
    "MarketRisk_FX",
    "Liquidity_Check"
]

DOCUMENTS = [
    "transactions_daily",
    "loans_portfolio",
    "currency_rates"
]

AUTHORS = ["user_ivan", "user_anna", "user_max", "system_scheduler"]

def generate_requests(count=20):
    requests_list = []
    base_date = datetime(2026, 11, 24)

    for _ in range(count):
        target_date = (base_date - timedelta(days=random.randint(0, 5))).strftime("%Y-%m-%d")
        req = {
            "author": random.choice(AUTHORS),
            "date": target_date,
            "document": random.choice(DOCUMENTS),
            "process": random.choice(PROCESSES),
            "priority": random.choice(["manual", "auto"])
        }
        requests_list.append(req)

    requests_list.append({
        "author": "user_vlad",
        "date": "2026-11-24",
        "document": "loans_portfolio",
        "process": "CreditRisk_Calc",
        "priority": "manual"
    })
    requests_list.append({
        "author": "user_nikita",
        "date": "2026-11-24",
        "document": "loans_portfolio",
        "process": "CreditRisk_Calc",
        "priority": "auto"
    })

    with open("test_requests.json", "w", encoding="utf-8") as f:
        json.dump(requests_list, f, ensure_ascii=False, indent=4)

    print(f"Done: {len(requests_list)} requests saved to test_requests.json")

if __name__ == "__main__":
    generate_requests(15)
