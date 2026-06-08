import pytest
from httpx import AsyncClient, ASGITransport
from unittest.mock import AsyncMock, patch

from app.main import app
from app.schemas import MoveResponse, MoveStats

INITIAL_BOARD = [4,4,4,4,4,4,0,4,4,4,4,4,4,0]


@pytest.fixture
def mock_motor_response():
    return MoveResponse(
        move=2,
        evaluation=1,
        elapsed_ms=50.0,
        stats=MoveStats(nodes=10000, prunes=3000),
        threads_used=1,
    )


@pytest.mark.asyncio
async def test_move_returns_200(mock_motor_response):
    with patch("app.main.call_motor", new_callable=AsyncMock, return_value=mock_motor_response):
        async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
            resp = await client.post("/move", json={
                "board": INITIAL_BOARD, "side": 0, "depth": 4, "threads": 1
            })
    assert resp.status_code == 200
    data = resp.json()
    assert "move" in data
    assert "evaluation" in data
    assert "stats" in data


@pytest.mark.asyncio
async def test_move_invalid_board_length():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
        resp = await client.post("/move", json={
            "board": [4,4,4], "side": 0, "depth": 4, "threads": 1
        })
    assert resp.status_code == 422


@pytest.mark.asyncio
async def test_move_invalid_side():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
        resp = await client.post("/move", json={
            "board": INITIAL_BOARD, "side": 5, "depth": 4, "threads": 1
        })
    assert resp.status_code == 422


@pytest.mark.asyncio
async def test_healthz():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
        resp = await client.get("/healthz")
    assert resp.status_code == 200
    assert resp.json()["status"] == "ok"


@pytest.mark.asyncio
async def test_readyz_motor_down():
    with patch("app.main.check_motor_health", new_callable=AsyncMock, return_value=False):
        async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
            resp = await client.get("/readyz")
    assert resp.status_code == 503


@pytest.mark.asyncio
async def test_cors_preflight():
    async with AsyncClient(transport=ASGITransport(app=app), base_url="http://test") as client:
        resp = await client.options("/move", headers={
            "Origin": "http://localhost:8080",
            "Access-Control-Request-Method": "POST",
            "Access-Control-Request-Headers": "Content-Type",
        })
    assert resp.status_code in (200, 204)
