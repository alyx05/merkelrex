#include "ApiServer.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

ApiServer::ApiServer(Database& database, OrderBook& ob)
    : db(database), orderBook(ob)
{
    registerRoutes();
}

namespace
{
    // Format a double as a fixed-precision decimal string, trimming trailing
    // zeros (but keeping at least one digit after the point) so we get clean
    // JSON numbers like "11094.1" instead of "11094.1000000000003638".
    std::string formatAmount(double value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(8) << value;
        std::string s = oss.str();

        size_t lastNonZero = s.find_last_not_of('0');
        if (s[lastNonZero] == '.') lastNonZero++; // keep one digit after the point
        s.erase(lastNonZero + 1);
        return s;
    }
}

void ApiServer::broadcast(const std::string& message)
{
    std::lock_guard<std::mutex> lock(wsMutex);
    for (auto* conn : wsConnections)
    {
        conn->send_text(message);
    }
}

void ApiServer::registerRoutes()
{
    CROW_ROUTE(app, "/api/v1/wallet")
    ([this](const crow::request& req) {
        auto userId = req.url_params.get("user_id");
        if (!userId)
        {
            return crow::response(400, "Missing required query param: user_id");
        }

        std::map<std::string, double> balances = db.loadWalletBalances(userId);

        std::ostringstream json;
        json << "{\"user_id\":\"" << userId << "\",\"balances\":{";
        bool first = true;
        for (const auto& [currency, amount] : balances)
        {
            if (!first) json << ",";
            json << "\"" << currency << "\":" << formatAmount(amount);
            first = false;
        }
        json << "}}";

        crow::response res(json.str());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/v1/orderbook")
    ([this](const crow::request& req) {
        auto pairParam = req.url_params.get("pair");
        if (!pairParam)
        {
            return crow::response(400, "Missing required query param: pair");
        }
        std::string pair = pairParam;

        std::vector<OrderBookEntry> bids = orderBook.getOrdersByProduct(OrderBookType::bid, pair);
        std::vector<OrderBookEntry> asks = orderBook.getOrdersByProduct(OrderBookType::ask, pair);

        auto aggregate = [](std::vector<OrderBookEntry>& entries) {
            std::map<double, double> levels;
            for (const auto& e : entries) levels[e.price] += e.amount;
            return levels;
        };

        std::map<double, double> bidLevels = aggregate(bids);
        std::map<double, double> askLevels = aggregate(asks);

        const size_t MAX_LEVELS = 20;

        std::ostringstream json;
        json << "{\"pair\":\"" << pair << "\",\"bids\":[";
        size_t count = 0;
        bool first = true;
        for (auto it = bidLevels.rbegin(); it != bidLevels.rend() && count < MAX_LEVELS; ++it, ++count)
        {
            if (!first) json << ",";
            json << "{\"price\":" << formatAmount(it->first) << ",\"amount\":" << formatAmount(it->second) << "}";
            first = false;
        }
        json << "],\"asks\":[";
        count = 0;
        first = true;
        for (auto it = askLevels.begin(); it != askLevels.end() && count < MAX_LEVELS; ++it, ++count)
        {
            if (!first) json << ",";
            json << "{\"price\":" << formatAmount(it->first) << ",\"amount\":" << formatAmount(it->second) << "}";
            first = false;
        }
        json << "]}";

        crow::response res(json.str());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/v1/orders").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        crow::json::rvalue body;
        try
        {
            body = crow::json::load(req.body);
            if (!body) throw std::runtime_error("invalid JSON");
        }
        catch (...)
        {
            return crow::response(400, "Malformed JSON body");
        }

        if (!body.has("user_id") || !body.has("product") || !body.has("type") ||
            !body.has("price") || !body.has("amount"))
        {
            return crow::response(400, "Missing required fields: user_id, product, type, price, amount");
        }

        std::string userId = body["user_id"].s();
        std::string product = body["product"].s();
        std::string typeStr = body["type"].s();
        double price = body["price"].d();
        double amount = body["amount"].d();

        if (price < 0 || amount < 0)
        {
            return crow::response(400, "price and amount must be non-negative");
        }

        User owner;
        if (!db.loadUser(userId, owner))
        {
            return crow::response(404, "No such user_id");
        }

        OrderBookType type;
        if (typeStr == "bid") type = OrderBookType::bid;
        else if (typeStr == "ask") type = OrderBookType::ask;
        else return crow::response(400, "type must be \"bid\" or \"ask\"");

        Wallet userWallet;
        for (const auto& [currency, bal] : db.loadWalletBalances(userId))
        {
            userWallet.insertCurrency(currency, bal);
        }

        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&now_time);
        std::ostringstream tsStream;
        tsStream << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
        std::string timestamp = tsStream.str();

        OrderBookEntry order{price, amount, timestamp, product, type, userId};

        if (!userWallet.canFulfillOrder(order))
        {
            // Crow v1.2.0 doesn't recognize 422 in its status-code table (silently
            // downgrades to 500 regardless of how it's set), so we use 400 instead --
            // a perfectly standard choice for "request rejected due to business rule".
            return crow::response(400, "Insufficient funds to place this order");
        }

        // old:
        // orderBook.insertOrder(order);
        // ... ticker broadcast ...
        // std::vector<OrderBookEntry> sales = orderBook.matchAsksToBidsLive(product);

        // new:
        std::vector<OrderBookEntry> sales = orderBook.matchNewOrder(order);

        // only rest whatever amount, if any, wasn't immediately filled
        if (order.amount > 0)
        {
            orderBook.insertOrder(order);
        }

        std::ostringstream ticker;
        ticker << "{\"type\":\"ticker\",\"product\":\"" << product
            << "\",\"side\":\"" << typeStr
            << "\",\"price\":" << formatAmount(price)
            << ",\"amount\":" << formatAmount(amount) << "}";
        broadcast(ticker.str());

        for (const auto& sale : sales)
        {
            db.settleSale(sale);

            std::ostringstream fillMsg;
            fillMsg << "{\"type\":\"fill\",\"product\":\"" << sale.product
                    << "\",\"side\":\"" << (sale.orderType == OrderBookType::asksale ? "ask" : "bid")
                    << "\",\"price\":" << formatAmount(sale.price)
                    << ",\"amount\":" << formatAmount(sale.amount)
                    << ",\"timestamp\":\"" << sale.timestamp << "\"}";
            broadcast(fillMsg.str());
        }

        std::ostringstream json;
        json << "{\"status\":\"accepted\",\"product\":\"" << product
            << "\",\"type\":\"" << typeStr
            << "\",\"price\":" << formatAmount(price)
            << ",\"amount\":" << formatAmount(amount) << "}";

        crow::response res(201, json.str());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/v1/analytics/ohlc")
    ([this](const crow::request& req) {
        auto pairParam = req.url_params.get("pair");
        if (!pairParam)
        {
            return crow::response(400, "Missing required query param: pair");
        }
        std::string pair = pairParam;

        auto startParam = req.url_params.get("start_date");
        auto endParam = req.url_params.get("end_date");
        std::string startDate = startParam ? startParam : "";
        std::string endDate = endParam ? endParam : "";

        std::vector<OHLCEntry> askStats = orderBook.getOHLC(OrderBookType::ask, pair, startDate, endDate);
        std::vector<OHLCEntry> bidStats = orderBook.getOHLC(OrderBookType::bid, pair, startDate, endDate);

        auto serialize = [](const std::vector<OHLCEntry>& stats) {
            std::ostringstream json;
            json << "[";
            bool first = true;
            for (const auto& e : stats)
            {
                if (!first) json << ",";
                json << "{\"date\":\"" << e.date
                     << "\",\"open\":" << formatAmount(e.open)
                     << ",\"high\":" << formatAmount(e.high)
                     << ",\"low\":" << formatAmount(e.low)
                     << ",\"close\":" << formatAmount(e.close) << "}";
                first = false;
            }
            json << "]";
            return json.str();
        };

        std::ostringstream json;
        json << "{\"pair\":\"" << pair << "\",\"asks\":" << serialize(askStats)
             << ",\"bids\":" << serialize(bidStats) << "}";

        crow::response res(json.str());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/ws/market-feed")
        .websocket(&app)
        .onopen([this](crow::websocket::connection& conn) {
            std::lock_guard<std::mutex> lock(wsMutex);
            wsConnections.insert(&conn);
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
            std::lock_guard<std::mutex> lock(wsMutex);
            wsConnections.erase(&conn);
        })
        .onmessage([](crow::websocket::connection& /*conn*/, const std::string& /*data*/, bool /*is_binary*/) {
            // This feed is broadcast-only (server -> client); incoming client
            // messages aren't part of the brief, so just ignore them.
        });
}

void ApiServer::run(int port)
{
    app.port(port).multithreaded().run();
}