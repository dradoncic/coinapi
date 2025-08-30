# Coinbase Market & Account Feed API

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
         ┌────────────┐ ┌────────────┐ ┌────────────┐
         │ Worker 0   │ │ Worker 1   │ │ Worker 7   │
         │ parse JSON │ │ normalize  │ │ validate   │
         │ validate   │ │ build diff │ │ snapshot   │
         └──────┬─────┘ └──────┬─────┘ └──────┬─────┘
                │               │               │
                ▼               ▼               ▼
       ┌─────────────────────────────────────────────┐
       │        Shared In-Memory Market State        │
       │  - order books (per product)                │
       │  - tickers / last trades                    │
       │  - status/heartbeat                         │
       │  (RCU snapshots / atomics / sharded locks)  │
       └───────────────┬─────────────────────────────┘
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
