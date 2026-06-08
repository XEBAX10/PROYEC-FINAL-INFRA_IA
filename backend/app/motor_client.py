import os
import httpx
from .schemas import MoveRequest, MoveResponse

MOTOR_URL = os.getenv("MOTOR_URL", "http://motor-svc:8001")
_TIMEOUT  = httpx.Timeout(60.0)


async def call_motor(req: MoveRequest) -> MoveResponse:
    payload = {
        "board":   req.board,
        "side":    req.side,
        "depth":   req.depth,
        "threads": req.threads,
    }
    async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
        resp = await client.post(f"{MOTOR_URL}/move", json=payload)
        resp.raise_for_status()
        return MoveResponse(**resp.json())


async def check_motor_health() -> bool:
    try:
        async with httpx.AsyncClient(timeout=httpx.Timeout(3.0)) as client:
            resp = await client.get(f"{MOTOR_URL}/healthz")
            return resp.status_code == 200
    except Exception:
        return False


async def get_motor_metrics() -> str:
    async with httpx.AsyncClient(timeout=_TIMEOUT) as client:
        resp = await client.get(f"{MOTOR_URL}/metrics")
        resp.raise_for_status()
        return resp.text
