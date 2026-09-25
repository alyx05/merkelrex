#pragma once

#include <vector>
#include "OrderBookEntry.h"
#include "OrderBook.h"
#include "Wallet.h"
#include "OHLCEntry.h"
#include "User.h"
#include "Database.h"

class MerkelMain
{
public:
    MerkelMain();
    void init();

private:
    void printMenu();
    int getUserOption();
    void processUserOption(int userOption);

    void showAuthMenu();
    void handleRegister();
    bool handleLogin();
    void handlePasswordReset();
    void migrateLegacyCsvData();   // one-time CSV -> SQLite import, runs once at startup

    void printHelp();
    void printMarketStats();
    void enterAsk();
    void enterBid();
    void printWallet();
    void gotoNextTimeframe();
    void handleOHLC();
    void syncWalletFile();
    void logTransaction(const std::string& type, const std::string& product, double price, double amount);
    void handleWalletAdjustments();
    void showRecentTransactions();
    void showUserActivityStats();
    void runTradingSimulation();

    std::string currentTime;

    Database db{"data/merkelrex.db"};
    OrderBook orderBook{"data/20200601.csv", db};
    Wallet wallet;

    bool isLoggedIn = false;
    User currentUserProfile;
};