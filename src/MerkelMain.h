#pragma once

#include <vector>
#include "OrderBookEntry.h"
#include "OrderBook.h"
#include "Wallet.h"
#include "OHLCEntry.h"
#include "User.h"

class MerkelMain
{
public:
    MerkelMain();
    // start the simulator
    void init();

private:
    void printMenu();
    int getUserOption();
    void processUserOption(int userOption);

    void showAuthMenu();
    bool userExists(const std::string& fullName, const std::string& email);
    bool loadUser(const std::string& username, User& outUser);
    void saveUserToFile(const User& user);
    void saveInitialWallet(const std::string& username, double bonusAmount);
    bool loadWalletFromFile(const std::string& username);
    void handleRegister();
    bool handleLogin();
    void handlePasswordReset();

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

    // OrderBook orderBook{"20200317.csv"};
    OrderBook orderBook{"src/20200601.csv"};
    Wallet wallet;

    bool isLoggedIn = false;
    User currentUserProfile;
};