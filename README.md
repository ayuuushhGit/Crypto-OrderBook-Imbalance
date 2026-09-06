# Crypto Order Book Imbalance (OBI) Engine

A C++ quantitative tool that connects to the Coinbase Exchange API to fetch real-time Level 2 order book snapshots and compute Order Book Imbalance (OBI). 

OBI is a standard market microstructure metric used to gauge short-term directional price pressure by measuring the normalized difference between resting bid and ask volume.

## Features
* **Live Microstructure Analysis:** Calculates top-of-book and multi-level OBI metrics.
* **Continuous Streaming:** Logs timestamps, mid-prices, spreads, and imbalances directly to a CSV feature store for downstream predictive modeling.
* **Modern C++:** Built using C++20 standards and CMake dependency management.

## Dependencies
This project uses CMake `FetchContent` to download and link dependencies automatically at build time. No manual installation is required.
* **cpr (C++ Requests):** Used for robust HTTP GET requests to the Coinbase REST API. (Forced static linking to avoid DLL missing errors on Windows).
* **nlohmann/json:** Header-only library used to parse the exchange's Level 2 market data payloads.

## Build Instructions (Windows / MSVC)
1. Clone the repository:
   ```bash
   git clone [https://github.com/ayuuushhGit/Crypto-OrderBook-Imbalance.git](https://github.com/ayuuushhGit/Crypto-OrderBook-Imbalance.git)
   cd Crypto-OrderBook-Imbalance
