FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ python3 python3-pip make ca-certificates && rm -rf /var/lib/apt/lists/*
RUN python3 -m pip install --break-system-packages --no-cache-dir sympy==1.14.0
WORKDIR /work
COPY . /work
CMD ["./scripts/quick_verify.sh"]
