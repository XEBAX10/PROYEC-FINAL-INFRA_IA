FROM gcc:13 AS builder

RUN apt-get update && apt-get install -y \
    cmake \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel $(nproc) --target mancala_server mancala_bench test_board

# Run tests during build
RUN cd build && ctest --output-on-failure

# ── Runtime image ──────────────────────────────────────────────────────────────
# Use gcc:13 base to guarantee libstdc++ version compatibility
FROM gcc:13

RUN apt-get update && apt-get install -y \
    libgomp1 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/mancala_server .
COPY --from=builder /build/build/mancala_bench  .
COPY --from=builder /build/tests/suite.txt       tests/
# Ensure libstdc++ matches the compiler version used at build time
COPY --from=builder /usr/lib/x86_64-linux-gnu/libstdc++.so.6* /usr/lib/x86_64-linux-gnu/
RUN ldconfig

ENV MOTOR_PORT=8001
ENV OMP_NUM_THREADS=4

EXPOSE 8001

HEALTHCHECK --interval=10s --timeout=3s --retries=3 \
    CMD curl -f http://localhost:8001/healthz || exit 1

# cache-bust: 2026-06-08
CMD ["./mancala_server"]
