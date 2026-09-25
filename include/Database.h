#pragma once

#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include "OrderBookEntry.h"
#include "User.h"
#include "Wallet.h"

struct TransactionRecord
{
    std::string username;
    std::string timestamp;
    std::string product;
    std::string type;
    double price;
    double amount;
    double total;
};

class Database
{
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool userExists(const std::string& fullName, const std::string& email);
    bool loadUser(const std::string& username, User& outUser);
    void saveUser(const User& user);
    void updatePassword(const std::string& username, size_t newPasswordHash);

    void saveInitialWallet(const std::string& username, double bonusAmount);
    std::map<std::string, double> loadWalletBalances(const std::string& username);
    void syncWalletBalances(const std::string& username,
                             const std::map<std::string, double>& balances);

    void logTransaction(const TransactionRecord& tx);
    std::vector<TransactionRecord> getRecentTransactions(const std::string& username, int limit);
    std::vector<TransactionRecord> getTransactionsForUser(const std::string& username,
                                                            const std::string& productFilter,
                                                            const std::string& startDate,
                                                            const std::string& endDate);

    std::vector<OrderBookEntry> getAllSaleEntries();

    // Apply a matched sale's currency movement to the named user's persisted
    // wallet and log it as a transaction. Shared by both the CLI (via
    // MerkelMain::gotoNextTimeframe) and the API (via ApiServer's orders
    // route) so a fill has exactly one settlement code path regardless of
    // where the order that triggered it came from. Silently does nothing if
    // `sale.username` isn't a real registered user (e.g. "dataset" rows from
    // the historical CSV, which have no wallet to settle against).
    void settleSale(const OrderBookEntry& sale);

private:
    sqlite3* db = nullptr;
    void initSchema();
    void exec(const std::string& sql);
};