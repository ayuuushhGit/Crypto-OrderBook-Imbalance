/**
 * @file CoinBaseDataFetch.cpp
 * @brief Continuous market data logger for BTC-USD Order Book Imbalance (OBI).
 *
 * This engine polls the Coinbase Exchange API to build a time-series dataset of
 * microstructure features. It calculates both top-of-book and multi-level depth
 * imbalances, saving the output to a CSV file for downstream predictive modeling.
 */
#define NOMINMAX // Add this at the very top before any includes!
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm> // Add this to define std::min
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

/**
 * @brief Calculates depth-weighted Order Book Imbalance across multiple price levels.
 *
 * @param bids JSON array of bid levels [price, size, num_orders]
 * @param asks JSON array of ask levels [price, size, num_orders]
 * @param levels The number of depth levels to aggregate (default: 5)
 * @return A double between -1.0 (sell pressure) and 1.0 (buy pressure).
 */
double calculate_multilevel_obi(const json& bids, const json& asks, int levels = 5) {
    double total_bid_vol = 0.0;
    double total_ask_vol = 0.0;

    int n_bids = std::min((int)bids.size(), levels);
    int n_asks = std::min((int)asks.size(), levels);

    for (int i = 0; i < n_bids; ++i) {
        total_bid_vol += std::stod(bids[i][1].get<std::string>());
    }
    for (int i = 0; i < n_asks; ++i) {
        total_ask_vol += std::stod(asks[i][1].get<std::string>());
    }

    // Safety check against zero liquidity to prevent division by zero
    if (total_bid_vol + total_ask_vol == 0.0) return 0.0;
    return (total_bid_vol - total_ask_vol) / (total_bid_vol + total_ask_vol);
}

int main() {
    // Initialize feature store file
    std::ofstream file("orderbook_features.csv", std::ios::out);
    file << "timestamp_ms,best_bid,best_ask,mid_price,spread,top_obi,multi_obi_5\n";
    file.flush();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Starting Quantitative Data Collection Engine...\n";
    std::cout << "Logging live BTC-USD features to orderbook_features.csv\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    // Continuous event loop
    while (true) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        cpr::Response r = cpr::Get(cpr::Url{ "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2" },
            cpr::Header{ {"User-Agent", "QuantDataFetch/1.0"} });

        if (r.status_code == 200) {
            try {
                auto data = json::parse(r.text);

                // Top-of-book extraction
                double bid_p = std::stod(data["bids"][0][0].get<std::string>());
                double bid_v = std::stod(data["bids"][0][1].get<std::string>());
                double ask_p = std::stod(data["asks"][0][0].get<std::string>());
                double ask_v = std::stod(data["asks"][0][1].get<std::string>());

                // Derived Microstructure Features
                double mid = (bid_p + ask_p) / 2.0;
                double spread = ask_p - bid_p;

                // Top-level OBI with division-by-zero safeguard
                double top_obi = 0.0;
                if (bid_v + ask_v > 0.0) {
                    top_obi = (bid_v - ask_v) / (bid_v + ask_v);
                }

                // Multi-level OBI
                double multi_obi = calculate_multilevel_obi(data["bids"], data["asks"], 5);

                // Console output for monitoring
                std::cout << "[" << ms << "] Mid: $" << mid
                    << " | Spread: $" << spread
                    << " | Top OBI: " << std::setprecision(4) << top_obi
                    << " | 5-Level OBI: " << multi_obi << "\n"
                    << std::setprecision(2);

                // Write directly to CSV
                file << ms << "," << bid_p << "," << ask_p << "," << mid << ","
                    << spread << "," << top_obi << "," << multi_obi << "\n";
                file.flush(); // Ensure data saves immediately in case of a crash

            }
            catch (const std::exception& e) {
                std::cerr << "Parsing error (skipping tick): " << e.what() << "\n";
            }
        }
        else {
            std::cerr << "HTTP " << r.status_code << " error. Retrying...\n";
        }

        // Rest for 1 second to comply with REST API rate limits (avoid getting IP banned)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}