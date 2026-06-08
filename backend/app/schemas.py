from pydantic import BaseModel, Field, field_validator
from typing import Annotated

class MoveRequest(BaseModel):
    board:   Annotated[list[int], Field(min_length=14, max_length=14)]
    side:    Annotated[int,       Field(ge=0, le=1)]
    depth:   Annotated[int,       Field(ge=1, le=20)]   = 8
    threads: Annotated[int,       Field(ge=1, le=16)]   = 1

    @field_validator("board")
    @classmethod
    def seeds_non_negative(cls, v: list[int]) -> list[int]:
        if any(x < 0 for x in v):
            raise ValueError("All board values must be >= 0")
        if sum(v) not in range(1, 49):
            raise ValueError("Total seeds must be between 1 and 48")
        return v

class MoveStats(BaseModel):
    nodes:  int
    prunes: int

class MoveResponse(BaseModel):
    move:         int
    evaluation:   int
    elapsed_ms:   float
    stats:        MoveStats
    threads_used: int

class HealthResponse(BaseModel):
    status: str

class ErrorResponse(BaseModel):
    error: str
