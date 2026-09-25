#pragma once

#include "crow.h"
#include "Database.h"
#include "OrderBook.h"
#include "Wallet.h"
#include <unordered_set>
#include <mutex>

class ApiServer
{
public:
    ApiServer(Database& database, OrderBook& orderBook);
    void run(int port);

private:
    Database& db;
    OrderBook& orderBook;
    crow::SimpleApp app;

    std::unordered_set<crow::websocket::connection*> wsConnections;
    std::mutex wsMutex;

    void registerRoutes();
    void broadcast(const std::string& message);
};