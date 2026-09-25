# Merkelrex — Base44 dev environment

## What this is
A C++17 order-book trading engine exposing a JSON REST + WebSocket API via the
[Crow](https://github.com/CrowCpp/Crow) framework, backed by SQLite3. There is
no frontend — the preview shows an API landing page at `/`.

## Stack
- **Language:** C++17, built with CMake (>= 3.15).
- **Web framework:** Crow v1.2.0, fetched at configure time via CMake
  `FetchContent` (needs git + network during `cmake` configure).
- **Storage:** SQLite3 (`data/merkelrex.db`) plus a historical CSV dataset
  (`data/20200601.csv`, ~60 MB) loaded into memory at startup.
- **No external credentials required** — all data is local files.

## Running
```
docker compose -f docker-compose.base44.yml up -d --build
```
The compose service builds from `Dockerfile.dev` (gcc:13 + cmake/sqlite3/git),
bind-mounts the repo at `/app`, configures/builds with CMake, then runs
`./build/merkelrex`. The app listens on **18080** inside the container, mapped
to host **3000** for the preview.

## No hot reload
C++ is compiled, so edits are **not** reflected live. After changing source:
```
docker compose -f docker-compose.base44.yml restart app
```
(re-runs the build + launch command), then `reload_preview`.

## Key endpoints
- `GET /` — API landing page (preview entry point)
- `GET /api/v1/wallet?user_id=<id>`
- `GET /api/v1/orderbook?pair=BTC/USDT`
- `POST /api/v1/orders` (JSON: user_id, product, type, price, amount)
- `GET /api/v1/analytics/ohlc?pair=BTC/USDT`
- `WS /ws/market-feed`

## Notes
- `main.cpp` runs the API server by default; pass `--cli` for the interactive
  console trading mode (not used in the preview).
- The app uses **relative paths** (`data/...`), so the binary must run with the
  repo root as its working directory (`/app`).
- `build/` is a named compose volume so CMake artifacts survive restarts.
