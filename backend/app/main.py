import os
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import httpx

from .schemas import MoveRequest, MoveResponse, HealthResponse
from .motor_client import call_motor, check_motor_health, get_motor_metrics

app = FastAPI(title="Mancala Backend", version="1.0.0")

# ── CORS ──────────────────────────────────────────────────────────────────────
_ORIGINS = [o.strip() for o in os.getenv(
    "CORS_ORIGINS",
    "http://localhost:8080,http://frontend-svc"
).split(",") if o.strip()]

app.add_middleware(
    CORSMiddleware,
    allow_origins=_ORIGINS,
    allow_methods=["GET", "POST", "OPTIONS"],
    allow_headers=["Content-Type"],
    allow_credentials=False,
)

# ── Endpoints ─────────────────────────────────────────────────────────────────

@app.post("/move", response_model=MoveResponse)
async def move(req: MoveRequest) -> MoveResponse:
    try:
        return await call_motor(req)
    except httpx.ConnectError:
        raise HTTPException(status_code=503, detail="Motor not available")
    except httpx.HTTPStatusError as e:
        raise HTTPException(status_code=502, detail=str(e))


@app.get("/healthz", response_model=HealthResponse)
async def healthz() -> HealthResponse:
    return HealthResponse(status="ok")


@app.get("/readyz", response_model=HealthResponse)
async def readyz() -> HealthResponse:
    if not await check_motor_health():
        raise HTTPException(status_code=503, detail="Motor not reachable")
    return HealthResponse(status="ok")


@app.get("/metrics")
async def metrics() -> str:
    try:
        return await get_motor_metrics()
    except Exception as e:
        raise HTTPException(status_code=503, detail=str(e))
