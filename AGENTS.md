# Merkelrex — Base44 dev environment

## What this is
A C++17 order-book trading engine with a React/Vite frontend dashboard.
The C++ backend exposes a JSON REST + WebSocket API (Crow + SQLite3); the
frontend is a Vite + React + Tailwind SPA served on the preview port.

## Architecture (two compose services)
- **`api`** — C++ backend. Built from `Dockerfile.dev` (gcc:13 + cmake/sqlite3/
  libasio-dev/git), bind-mounts the repo at `/app`, builds via CMake (fetches
  Crow v1.2.0 via FetchContent at configure time), runs `./build/merkelrex` on
  port **18080**. Not exposed to the host — the web service proxies to it.
- **`web`** — Vite + React + Tailwind frontend. Built from `frontend/Dockerfile.dev`
  (node:22-slim), bind-mounts `frontend/` at `/app`, runs Vite dev server on
  port **5173** mapped to host **3000**. Vite proxies `/api` and `/ws` to
  `http://api:18080` (single-origin, same as the old landing page approach).

## Running
```
docker compose -f docker-compose.base44.yml up -d --build
```

## Hot reload
- **Frontend (web):** Vite HMR — edits to `frontend/src/**` appear live.
- **Backend (api):** C++ is compiled, no hot reload. After source changes:
  `docker compose -f docker-compose.base44.yml restart api` then `reload_preview`.

## Backend key endpoints (proxied through Vite)
- `GET /api/v1/wallet?user_id=<id>`
- `GET /api/v1/orderbook?pair=BTC/USDT`
- `POST /api/v1/orders` (JSON: user_id, product, type, price, amount)
- `GET /api/v1/analytics/ohlc?pair=BTC/USDT`
- `WS /ws/market-feed`

## Default demo user
`6239683678` — has BTC, ETH, DOGE, USDT wallet balances in the SQLite DB.

## Available trading pairs
BTC/USDT, ETH/USDT, ETH/BTC, DOGE/USDT, DOGE/BTC

## Notes
- `main.cpp` runs the API server by default; pass `--cli` for interactive console
  mode (not used in the preview).
- The C++ binary uses **relative paths** (`data/...`), so it runs with CWD = `/app`.
- `build/` is a named compose volume so CMake artifacts survive restarts.
- `/app/node_modules` is an anonymous volume so the image-installed deps
  aren't shadowed by the bind mount.
