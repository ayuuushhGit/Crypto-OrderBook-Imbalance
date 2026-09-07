/**
 * @file CoinBaseDataFetch.cpp
 * @brief Continuous market data logger for BTC-USD Order Book Imbalance (OBI).
 */
#define NOMINMAX 
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm> 
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

/**
 * @brief Calculates depth-weighted Order Book Imbalance across multiple price levels.
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

    if (total_bid_vol + total_ask_vol == 0.0) return 0.0;
    return (total_bid_vol - total_ask_vol) / (total_bid_vol + total_ask_vol);
}

int main() {
    // --- RUNTIME CONFIGURATION ---
    int MAX_RUNTIME_SECONDS = 120;

    std::ofstream file("orderbook_features.csv", std::ios::out);
    // Added latency_ms to the CSV header
    file << "timestamp_ms,best_bid,best_ask,mid_price,spread,top_obi,multi_obi_5,latency_ms\n";
    file.flush();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Starting Quantitative Data Collection Engine...\n";

    if (MAX_RUNTIME_SECONDS > 0) {
        std::cout << "Engine configured to run for " << MAX_RUNTIME_SECONDS << " seconds.\n";
    }
    else {
        std::cout << "Engine configured to run infinitely.\n";
    }

    std::cout << "Logging live BTC-USD features to orderbook_features.csv\n";
    std::cout << "Press Ctrl+C to stop manually.\n\n";

    auto engine_start_time = std::chrono::steady_clock::now();

    while (true) {
        // --- TIMER CHECK ---
        if (MAX_RUNTIME_SECONDS > 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - engine_start_time).count();

            if (elapsed_seconds >= MAX_RUNTIME_SECONDS) {
                std::cout << "\nRuntime limit of " << MAX_RUNTIME_SECONDS << " seconds reached. Shutting down cleanly.\n";
                break;
            }
        }

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // Start the latency stopwatch
        auto fetch_start = std::chrono::steady_clock::now();

        cpr::Response r = cpr::Get(cpr::Url{ "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2" },
            cpr::Header{ {"User-Agent", "QuantDataFetch/1.0"} });

        if (r.status_code == 200) {
            try {
                auto data = json::parse(r.text);

                double bid_p = std::stod(data["bids"][0][0].get<std::string>());
                double bid_v = std::stod(data["bids"][0][1].get<std::string>());
                double ask_p = std::stod(data["asks"][0][0].get<std::string>());
                double ask_v = std::stod(data["asks"][0][1].get<std::string>());

                double mid = (bid_p + ask_p) / 2.0;
                double spread = ask_p - bid_p;

                double top_obi = 0.0;
                if (bid_v + ask_v > 0.0) {
                    top_obi = (bid_v - ask_v) / (bid_v + ask_v);
                }

                double multi_obi = calculate_multilevel_obi(data["bids"], data["asks"], 5);

                // Stop the latency stopwatch right after the math completes
                auto fetch_end = std::chrono::steady_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(fetch_end - fetch_start).count();

                std::cout << "[" << ms << "] Mid: $" << mid
                    << " | Spread: $" << spread
                    << " | Top OBI: " << std::setprecision(4) << top_obi
                    << " | 5-Level OBI: " << multi_obi
                    << " | Latency: " << latency << " ms\n"
                    << std::setprecision(2);

                // Append latency to CSV
                file << ms << "," << bid_p << "," << ask_p << "," << mid << ","
                    << spread << "," << top_obi << "," << multi_obi << "," << latency << "\n";
                file.flush();

            }
            catch (const std::exception& e) {
                std::cerr << "Parsing error (skipping tick): " << e.what() << "\n";
            }
        }
        else {
            std::cerr << "HTTP " << r.status_code << " error. Retrying...\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}