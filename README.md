# Coinbase Market & Account Feed Infrastructure (C++17)

A lightweight, high-performance trading infrastructure written in **C++17**.  
Built for **quantitative research & strategy development**, starting with Coinbase WebSocket/REST APIs and extendable to multiple exchanges.

---

## Features

- **Market Data Ingestion**
  - Connects to Coinbase WebSocket feed via **Boost.Beast/Asio + OpenSSL**.
  - Subscribes to **8 channels** (ticker, matches, level2, etc.).
  - Single connection, async demux into per-channel lock-free queues.

- **High-performance Parsing**
  - Uses **simdjson** for ultra-fast JSON parsing.
  - Validates sequence numbers and detects feed gaps.
  - Normalizes into snapshots/diffs for order books and tickers.

- **Shared In-Memory State**
  - Lock-free / RCU-style snapshots of order books and trades.
  - Strategies can read state without blocking workers.
  - Ready for cross-product & multi-exchange extensions.

- **Trading API Wrapper**
  - Clean C++ class wrapping Coinbase REST (user/trading/portfolio).
  - Pluggable interface for adding Binance, Kraken, etc.

- **Persistence**
  - Redis buffer for real-time pub/sub & short-term storage.
  - Postgres writer thread for long-term historical storage & replay.

- **Strategy Layer**
  - Strategies run as independent threads consuming read-only snapshots.
  - Example strategies: mean reversion, imbalance, cross-exchange arbitrage.

---

## Architecture


```
                         ┌─────────────────────┐
                         │   Coinbase WS API   │
                         └─────────┬───────────┘
                                   │
                        JSON frames (mixed channels)
                                   │
                         ┌─────────────────────┐
                         │  I/O Thread (Boost) │
                         │  - async_read loop  │
                         │  - demux by channel │
                         └─────────┬───────────┘
                                   │
                                   ▼
              ┌───────────────────────────────────────┐
              │   Per-Channel Lock-Free Queues (8x)   │
              │   (ticker, matches, l2updates, etc.)  │
              └─────────┬─────────┬─────────┬─────────┘
                        │         │         │
                        ▼         ▼         ▼
       ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐
       │ Worker 0   │ │ Worker 1   │ │ Worker ... │ │ Worker 7   │
       │ parse JSON │ │ normalize  │ │ validate   │ │ maintain   │
       │ validate   │ │ build diff │ │ snapshot   │ │ heartbeat  │
       └──────┬─────┘ └──────┬─────┘ └─────┬────-─┘ └──────┬─────┘
              │              │             │               │
              ▼              ▼             ▼               ▼
       ┌────────────────────────────────────────────────────────┐
       │        Shared In-Memory Market State                   │
       │  - order books (per product)                           │
       │  - tickers / last trades                               │
       │  - status/heartbeat                                    │
       │  (RCU snapshots / atomics / sharded locks)             │
       └───────────────────────────┬────────────────────────────┘
                                   │
                      Read-only    │      Write diffs
                 ┌─────────────────┼───────────────────┐
                 │                 │                   │
                 ▼                 ▼                   ▼
            ┌─────────────┐  ┌─────────────┐    ┌───────────────┐
            │ Strategy A  │  │ Strategy B  │    │ Strategy N    │
            │  (mean revr)│  │  (imbalance)│    │  (arbitrage)  │
            └─────┬───────┘  └──────┬──────┘    └──────┬────────┘
                  │                 │                  │
                  └───────┬─────────┴───────────┬──────┘
                          │                     │
                          ▼                     ▼
             ┌──────────────-───┐    ┌──────────────────────┐
             │ Order Manager    │    │ Portfolio/Risk Model │
             │ - wraps REST/WS  │    │ - balances/positions │
             │ - sends/cancels  │    │ - exposure checks    │
             └─────────┬────────┘    └─────────┬────────────┘
                       │                       │
                       ▼                       ▼
           ┌─────────────────────┐   ┌─────────────────────┐
           │ Coinbase User API   │   │ (Future: Other CEXs)│
           └─────────────────────┘   └─────────────────────┘


              ┌────────────────────────────────────────┐
              │ Persistence / Replay Layer             │
              │                                        │
              │  Redis buffer (real-time stream):      │
              │    - every tick/orderbook diff         │
              │    - low-latency pub/sub for consumers │
              │                                        │
              │  Postgres historical DB:               │
              │    - bulk store (partitioned by date)  │
              │    - supports historical replay        │
              └────────────────────────────────────────┘
```
---

## Persistence:
- **Redis**: fast buffer of normalized ticks/order book diffs.  
- **Postgres**: long-term storage & historical replay.

---

##  Build Instructions

### Requirements
- **clang++** (>= 10, with C++17 support)
- **Boost** (>= 1.78) — Beast, Asio
- **OpenSSL** (>= 1.1)
- **simdjson** (>= 3.0)
- **Redis client library** (optional, for persistence)
- **libpq** (optional, for Postgres)

### Build

```bash
# Clone repository
git clone https://github.com/dradoncic/coinapi.git
cd coinapi

# Example build (no cmake yet)
clang++ -std=c++17 -O3 -Wall     -I/path/to/boost/include     -I/path/to/simdjson/include     -L/path/to/boost/lib     -L/usr/lib     -lssl -lcrypto -lpthread     src/main.cpp -o coinapi
```

Or via **CMake** (recommended):

```bash
mkdir build && cd build
cmake ..
make -j
```

---

## Usage

1. **Run the core market data service**
   ```bash
   ./coinapi --config config.json
   ```

2. **Inspect live Redis/Postgres writes**  
   - Redis streams for real-time monitoring.  
   - Postgres for historical queries & replay.

3. **Develop strategies** by subclassing the `Strategy` interface.  
   Strategies receive immutable snapshots of books/prices and can send orders via the `OrderManager`.

---

## Roadmap

- [x] Coinbase market data feed
- [x] Lock-free queue → worker → shared state
- [x] simdjson parsing
- [x] Trading API wrapper
- [ ] Redis buffer integration
- [ ] Postgres persistence & replay engine
- [ ] Multi-exchange (Binance, Kraken)
- [ ] Strategy plugin system
- [ ] Monitoring dashboard (Grafana/Prometheus)
