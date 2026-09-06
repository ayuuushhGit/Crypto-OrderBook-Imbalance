/**
 * @file CoinBaseDataFetch.cpp
 * @brief Fetches a Level 2 order book snapshot from Coinbase and calculates top-of-book imbalance.
 *
 * This script connects to the public Coinbase REST API to retrieve current resting
 * liquidity for BTC-USD. It extracts the best bid and ask to compute the Order
 * Book Imbalance (OBI), a standard microstructure metric for gauging short-term
 * directional price pressure.
 */

#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

int main() {
    std::cout << "Requesting live Level-2 Order Book from Coinbase..." << std::endl;

    // Fetch Level 2 snapshot (returns the top 50 bids and asks).
    // NOTE: A custom User-Agent is passed to prevent the request from being 
    // flagged and dropped by Coinbase's Cloudflare security layer.
    cpr::Response r = cpr::Get(cpr::Url{ "https://api.exchange.coinbase.com/products/BTC-USD/book?level=2" },
        cpr::Header{ {"User-Agent", "QuantDataFetch/1.0"} });

    // Fail-fast on network issues. Public endpoints are strictly rate-limited 
    // (typically to 3-10 requests per second per IP).
    if (r.status_code != 200) {
        std::cerr << "Network Error: HTTP " << r.status_code << "\n" << r.text << std::endl;
        return 1;
    }

    try {
        auto parsed_data = json::parse(r.text);

        // Coinbase returns depth levels as arrays of strings to prevent floating-point precision loss.
        // Expected structure: [price (string), size (string), num_orders (integer)]
        // We extract Index 0 (price) and Index 1 (size), converting them to doubles for calculation. 
        // Index 2 (num_orders) is intentionally ignored as we are analyzing aggregate volume.

        double best_bid_price = std::stod(parsed_data["bids"][0][0].get<std::string>());
        double best_bid_vol = std::stod(parsed_data["bids"][0][1].get<std::string>());

        double best_ask_price = std::stod(parsed_data["asks"][0][0].get<std::string>());
        double best_ask_vol = std::stod(parsed_data["asks"][0][1].get<std::string>());

        std::cout << "--- LIVE BTC-USD MARKET DATA ---" << std::endl;
        std::cout << "Bid: $" << best_bid_price << " (" << best_bid_vol << " BTC)" << std::endl;
        std::cout << "Ask: $" << best_ask_price << " (" << best_ask_vol << " BTC)" << std::endl;

        // Calculate Top-of-Book Order Book Imbalance (OBI).
        // Normalizes the volume difference to a range of [-1.0, 1.0]. 
        // Positive values suggest buying pressure; negative values suggest selling pressure.
        // TODO: Add a check for (best_bid_vol + best_ask_vol == 0) to prevent division by zero 
        // in the event of an empty order book during an exchange outage.
        double obi = (best_bid_vol - best_ask_vol) / (best_bid_vol + best_ask_vol);

        std::cout << "Live Top-of-Book OBI: " << obi << std::endl;

    }
    catch (const std::exception& e) {
        // Captures JSON parsing errors (e.g., if the exchange returns an HTML error page instead of JSON)
        std::cerr << "Parsing Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}