// merkelmain.cpp

#include <iostream>
#include <vector>
#include <map>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "MerkelMain.h"
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include "OHLCEntry.h"
#include "User.h"

MerkelMain::MerkelMain()
{
}

void MerkelMain::init()
{
    migrateLegacyCsvData();

    int input;
    currentTime = orderBook.getEarliestTime();

// enforce authentication loop
    while (true)
    {
        while (!isLoggedIn) {
            showAuthMenu();
            std::string choiceLine;
            if (!std::getline(std::cin, choiceLine)) {
                std::cout << "\nInput ended. Exiting.\n" << std::endl;
                return;
            }
            int authChoice = 0;
            try { authChoice = std::stoi(choiceLine); } catch(...) {}

            if (authChoice == 1) handleLogin();
            else if (authChoice == 2) handleRegister();
            else if (authChoice == 3) handlePasswordReset();
            else if (authChoice == 4) {
                std::cout << "\nExiting system. Goodbye!" << std::endl;
                return;
            } else {
                std::cout << "Invalid choice. Please pick 1-4.\n" << std::endl;
            }
        }

        printMenu();
        input = getUserOption();
        if (input == -1) {
            std::cout << "\nInput ended. Exiting.\n" << std::endl;
            return;
        }
        if (input == 12) {
            std::cout << "\nLogout / Exit selected. Returning to auth screen.\n" << std::endl;
            isLoggedIn = false;
            continue;
        }
        processUserOption(input);
    }
}

void MerkelMain::printMenu()
{
    // print menu options
    std::cout << "1: Print help " << std::endl;
    std::cout << "2: Print exchange stats" << std::endl;
    std::cout << "3: Make an offer " << std::endl;
    std::cout << "4: Make a bid " << std::endl;
    std::cout << "5: Print wallet " << std::endl;
    std::cout << "6: Continue " << std::endl;
    std::cout << "7: View Product OHLC Data " << std::endl;
    std::cout << "8: Deposit or Withdraw Funds" << std::endl;
    std::cout << "9: View Recent Transactions" << std::endl;
    std::cout << "10: View Activity Summary Statistics" << std::endl;
    std::cout << "11: Run 2026 Trading Activity Simulator" << std::endl;
    std::cout << "12: Logout / Exit" << std::endl;
    std::cout << std::endl;

    // display system time (menu reflects now, not dataset time)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t tnow = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&tnow);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
        std::cout << "Current time is: " << oss.str() << "\n" << std::endl;
    }
}

int MerkelMain::getUserOption()
{
    int userOption = 0;
    std::string line;
    std::cout << "Type in 1-12" << std::endl;
    if (!std::getline(std::cin, line)) {
        return -1;
    }
    try
    {
        userOption = std::stoi(line);
    }
    catch (const std::exception &e)
    {
    }
    std::cout << "\nYou chose: " << userOption << std::endl;
    std::cout << std::endl;
    return userOption;
}

void MerkelMain::processUserOption(int userOption)
{
    if (userOption == 1)
    {
        printHelp();
    }
    else if (userOption == 2)
    {
        printMarketStats();
    }
    else if (userOption == 3)
    {
        enterAsk();
    }
    else if (userOption == 4)
    {
        enterBid();
    }
    else if (userOption == 5)
    {
        printWallet();
    }
    else if (userOption == 6)
    {
        gotoNextTimeframe();
    }
    else if (userOption == 7)
    {
        handleOHLC();
    }
    else if (userOption == 8)
    {
        handleWalletAdjustments();
    }
    else if (userOption == 9)
    {
        showRecentTransactions();
    }
    else if (userOption == 10)
    {
        showUserActivityStats();
    }
    else if (userOption == 11)
    {
        runTradingSimulation();
    }
    else // invalid option (0 or out-of-range)
    {
        std::cout << "Invalid choice. Choose 1-12\n" << std::endl;
    }
}

void MerkelMain::showAuthMenu()
{
    std::cout << "WELCOME TO MERKLEREX TRADING" << std::endl;
    std::cout << std::endl;
    std::cout << "1: Log In" << std::endl;
    std::cout << "2: Register New Profile" << std::endl;
    std::cout << "3: Reset Password" << std::endl;
    std::cout << "4: Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Choose an option: ";
}

void MerkelMain::handleRegister()
{
    std::string fullName, email, password;
    std::cout << "\nREGISTER NEW ACCOUNT" << std::endl;
    std::cout << "Enter Full Name: ";
    std::getline(std::cin, fullName);
    std::cout << "Enter Email Address: ";
    std::getline(std::cin, email);

    if (db.userExists(fullName, email)) {
        std::cout << "\nError: An account with this name and email already exists!\n" << std::endl;
        return;
    }

    std::cout << "Enter Password: ";
    std::getline(std::cin, password);

    std::srand(std::time(0));
    std::string newUsername;
    User existingUser;
    do {
        newUsername.clear();
        for (int i = 0; i < 10; ++i) {
            newUsername += std::to_string(std::rand() % 10);
        }
    } while (db.loadUser(newUsername, existingUser));

    std::hash<std::string> stringHasher;
    size_t hashedPass = stringHasher(password);

    User newUser{newUsername, fullName, email, hashedPass};
    db.saveUser(newUser);

    double signUpBonus = 5000.0;
    db.saveInitialWallet(newUsername, signUpBonus);
    wallet.insertCurrency("USDT", signUpBonus);

    std::cout << "\nRegistration Successful!\n" << std::endl;
    std::cout << "YOUR UNIQUE LOGIN USERNAME IS: " << newUsername << std::endl;
    std::cout << "Please write this down! You will need it to login.\n" << std::endl;
}

bool MerkelMain::handleLogin()
{
    std::string inputUser, inputPass;
    std::cout << "\nLOGIN" << std::endl;
    std::cout << "Enter 10-digit Username: ";
    std::getline(std::cin, inputUser);
    std::cout << "Enter Password: ";
    std::getline(std::cin, inputPass);

    User targetUser;
    if (db.loadUser(inputUser, targetUser)) {
        std::hash<std::string> stringHasher;
        if (stringHasher(inputPass) == targetUser.passwordHash) {
            isLoggedIn = true;
            currentUserProfile = targetUser;

            wallet.reset();   // <- clear any stale balances from a previous session before restoring
            std::map<std::string, double> balances = db.loadWalletBalances(currentUserProfile.username);
            if (balances.empty()) {
                std::cout << "(No prior wallet balance found - starting fresh.)" << std::endl;
            } else {
                for (const auto& [currency, amount] : balances) {
                    if (amount > 0) wallet.insertCurrency(currency, amount);
                }
            }

            std::cout << "\nWelcome back, " << targetUser.fullName << "! Login successful.\n" << std::endl;
            return true;
        }
    }

    std::cout << "\nInvalid username or password.\n" << std::endl;
    return false;
}

void MerkelMain::handlePasswordReset()
{
    std::string inputUser, inputEmail;
    std::cout << "\nPASSWORD RESET" << std::endl;
    std::cout << "Enter your 10-digit Username: ";
    std::getline(std::cin, inputUser);
    std::cout << "Confirm your Email Address: ";
    std::getline(std::cin, inputEmail);

    User targetUser;
    if (db.loadUser(inputUser, targetUser) && targetUser.email == inputEmail) {
        std::string newPassword;
        std::cout << "\nIdentity verified! Enter new password: ";
        std::getline(std::cin, newPassword);

        std::hash<std::string> stringHasher;
        size_t newHash = stringHasher(newPassword);
        db.updatePassword(inputUser, newHash);

        std::cout << "Password successfully updated! You can now log in.\n" << std::endl;
    } else {
        std::cout << "\nError: Username and email combination mismatch.\n" << std::endl;
    }
}

// One-time import: if legacy CSVs exist and the DB's `users` table is empty,
// pull old registrations/wallets/trades into SQLite so testing progress isn't lost.
void MerkelMain::migrateLegacyCsvData()
{
    std::ifstream registerFile("src/USERS_REGISTER.CSV");
    if (!registerFile.is_open()) return; // nothing to migrate

    std::string line;
    while (std::getline(registerFile, line))
    {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() < 4) continue;

        User legacyUser;
        legacyUser.username = tokens[0];
        legacyUser.fullName = tokens[1];
        legacyUser.email = tokens[2];
        legacyUser.passwordHash = std::stoull(tokens[3]);

        User existing;
        if (db.loadUser(legacyUser.username, existing)) continue; // already migrated

        db.saveUser(legacyUser);
    }

    std::ifstream walletFile("src/USERS_WALLET.CSV");
    if (walletFile.is_open())
    {
        std::map<std::string, std::map<std::string, double>> latestPerUser;
        while (std::getline(walletFile, line))
        {
            std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
            if (tokens.size() != 2) continue;
            std::vector<std::string> pairs = CSVReader::tokenise(tokens[1], '|');
            for (const std::string& p : pairs)
            {
                size_t colon = p.find(':');
                if (colon == std::string::npos) continue;
                try {
                    latestPerUser[tokens[0]][p.substr(0, colon)] = std::stod(p.substr(colon + 1));
                } catch (...) {}
            }
        }
        for (const auto& [username, balances] : latestPerUser)
        {
            if (db.loadWalletBalances(username).empty())
                db.syncWalletBalances(username, balances);
        }
    }

    std::ifstream tradingFile("src/USERS_TRADING.CSV");
    if (tradingFile.is_open())
    {
        int skipped = 0;
        while (std::getline(tradingFile, line))
        {
            std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
            if (tokens.size() < 7) continue;

            User owner;
            if (!db.loadUser(tokens[0], owner)) { skipped++; continue; } // orphan username -- not a real registered user

            TransactionRecord tx;
            tx.username = tokens[0];
            tx.timestamp = tokens[1];
            tx.product = tokens[2];
            tx.type = tokens[3];
            try {
                tx.price = std::stod(tokens[4]);
                tx.amount = std::stod(tokens[5]);
                tx.total = std::stod(tokens[6]);
            } catch (...) { continue; }
            db.logTransaction(tx);
        }
        if (skipped > 0)
            std::cout << "(Skipped " << skipped << " legacy trade record(s) with no matching registered user.)" << std::endl;
    }

    std::cout << "(Legacy CSV data migrated into SQLite.)" << std::endl;
}

void MerkelMain::printHelp()
{
    std::cout << "Help - your aim is to make money. Analyse the market and make bids and offers. " << std::endl;
    std::cout << std::endl;
}

void MerkelMain::printMarketStats()
{
    for (std::string const &p : orderBook.getKnownProducts())
    {
        std::cout << "Product: " << p << std::endl;
        std::vector<OrderBookEntry> entries = orderBook.getOrders(OrderBookType::ask,
                                                                  p, currentTime);
        std::cout << "Asks seen: " << entries.size() << std::endl;
        std::cout << "Max ask: " << OrderBook::getHighPrice(entries) << std::endl;
        std::cout << "Min ask: " << OrderBook::getLowPrice(entries) << std::endl;
        std::cout << std::endl;
    }
    // (legacy debug counters removed)
}

void MerkelMain::enterAsk()
{
    std::cout << "Make an ask - enter the amount: product,price,amount (e.g. ETH/BTC,200,0.5)." << std::endl;
    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if (tokens.size() != 3)
    {
        std::cout << std::endl;
        std::cout << "Bad input: " << input << std::endl;
        std::cout << std::endl;
    }
    else
    {
        try
        {
            OrderBookEntry obe = CSVReader::stringsToOBE(
                tokens[1],
                tokens[2],
                currentTime,
                tokens[0],
                OrderBookType::ask);
            obe.username = currentUserProfile.username;
            if (obe.price < 0 || obe.amount < 0) {
                std::cout << "Invalid input: price and amount must be non-negative." << std::endl;
                std::cout << std::endl;
            }
            else if (wallet.canFulfillOrder(obe))
            {
                std::cout << "Wallet looks good." << std::endl;
                std::cout << std::endl;
                orderBook.insertOrder(obe);
            }
            else
            {
                std::cout << "Wallet has insufficient funds." << std::endl;
                std::cout << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << " MerkelMain::enterAsk Bad input " << std::endl;
        }
    }
}

void MerkelMain::enterBid()
{
    std::cout << "Make a bid - enter the amount: product,price,amount (e.g.  ETH/BTC,200,0.5)." << std::endl;
    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if (tokens.size() != 3)
    {
        std::cout << std::endl;
        std::cout << "Bad input: " << input << std::endl;
        std::cout << std::endl;
    }
    else
    {
        try
        {
            OrderBookEntry obe = CSVReader::stringsToOBE(
                tokens[1],
                tokens[2],
                currentTime,
                tokens[0],
                OrderBookType::bid);
            obe.username = currentUserProfile.username;
            if (obe.price < 0 || obe.amount < 0) {
                std::cout << "Invalid input: price and amount must be non-negative." << std::endl;
                std::cout << std::endl;
            }
            else if (wallet.canFulfillOrder(obe))
            {
                std::cout << "Wallet looks good. " << std::endl;
                std::cout << std::endl;
                orderBook.insertOrder(obe);
            }
            else
            {
                std::cout << "Wallet has insufficient funds . " << std::endl;
                std::cout << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "Bad input!" << std::endl;
        }
    }
}

void MerkelMain::printWallet()
{
    std::cout << wallet.toString() << std::endl;
}

void MerkelMain::gotoNextTimeframe()
{
    std::cout << "Going to next time frame." << std::endl;
    std::cout << std::endl;
    for (std::string p : orderBook.getKnownProducts())
    {
        std::cout << "Matching " << p << std::endl;
        std::vector<OrderBookEntry> sales = orderBook.matchAsksToBids(p, currentTime);
        std::cout << "Sales: " << sales.size() << std::endl;
        std::cout << std::endl;
        for (OrderBookEntry &sale : sales)
        {
            std::cout << "Sale price: " << sale.price << " amount " << sale.amount << std::endl;

            db.settleSale(sale);

            // If this fill belongs to the currently logged-in CLI session's
            // user, refresh the live in-memory wallet from the DB so the UI
            // reflects it immediately -- db.settleSale already persisted the
            // authoritative balance.
            if (sale.username == currentUserProfile.username)
            {
                wallet.reset();
                for (const auto& [currency, amount] : db.loadWalletBalances(currentUserProfile.username))
                {
                    if (amount > 0) wallet.insertCurrency(currency, amount);
                }
            }
        }
    }

    currentTime = orderBook.getNextTime(currentTime);
}

void MerkelMain::handleOHLC()
{
    std::cout << "Enter product (e.g., ETH/BTC): ";
    std::string product;
    std::getline(std::cin, product);

    std::cout << "Filter by specific date range? (y/n): ";
    std::string filterChoice;
    std::getline(std::cin, filterChoice);

    std::string startDate = "";
    std::string endDate = "";

    if (filterChoice == "y" || filterChoice == "Y") {
        std::cout << "Enter start date (YYYY/MM/DD): ";
        std::getline(std::cin, startDate);
        std::cout << "Enter end date (YYYY/MM/DD): ";
        std::getline(std::cin, endDate);
    }

    // process asks
    std::cout << "\nOHLC Asks: " << product << std::endl;
    std::vector<OHLCEntry> askStats = orderBook.getOHLC(OrderBookType::ask, product, startDate, endDate);
    if (askStats.empty()) {
        std::cout << "No ask records found for this date range." << std::endl;
    } else {
        std::cout << "Date\t\tOpen\tHigh\tLow\tClose" << std::endl;
        for (const auto& entry : askStats) {
            std::cout << entry.date << "\t" << entry.open << "\t" << entry.high << "\t" << entry.low << "\t" << entry.close << std::endl;
        }
    }

    // process bids
    std::cout << "\nOHLC Bids: " << product << std::endl;
    std::vector<OHLCEntry> bidStats = orderBook.getOHLC(OrderBookType::bid, product, startDate, endDate);
    if (bidStats.empty()) {
        std::cout << "No bid records found for this date range." << std::endl;
    } else {
        std::cout << "Date\t\tOpen\tHigh\tLow\tClose" << std::endl;
        for (const auto& entry : bidStats) {
            std::cout << entry.date << "\t" << entry.open << "\t" << entry.high << "\t" << entry.low << "\t" << entry.close << std::endl;
        }
    }
    std::cout << "\n" << std::endl;
}

// Helper to get the actual current system time formatted like the dataset
std::string getSystemTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
    return oss.str();
}

// Persist the live in-memory wallet's balances for the current user.
// Replaces the old "rewrite USERS_WALLET.CSV" approach with an atomic
// delete+reinsert inside a SQLite transaction (see Database::syncWalletBalances).
void MerkelMain::syncWalletFile()
{
    std::map<std::string, double> balances = wallet.getBalances();
    db.syncWalletBalances(currentUserProfile.username, balances);
}

void MerkelMain::logTransaction(const std::string& type, const std::string& product, double price, double amount)
{
    TransactionRecord tx;
    tx.username = currentUserProfile.username;
    tx.timestamp = getSystemTimestamp();
    tx.product = product;
    tx.type = type;
    tx.price = price;
    tx.amount = amount;
    tx.total = price * amount;
    db.logTransaction(tx);
}

void MerkelMain::handleWalletAdjustments()
{
    std::cout << "WALLET DEPOSIT / WITHDRAWAL SIMULATOR" << std::endl;
    std::cout << "1: Deposit Funds\n2: Withdraw Funds" << std::endl;
    std::cout << std::endl;
    std::cout << "Choose choice: ";
    std::cout << std::endl;
    std::string selection;
    std::getline(std::cin, selection);

    std::cout << std::endl;
    std::cout << "Enter Currency token (e.g., USDT, BTC, ETH): ";
    std::string currency;
    std::getline(std::cin, currency);

    std::cout << "Enter Amount: ";
    std::string amountStr;
    std::getline(std::cin, amountStr);
    double amount = 0.0;
    try { amount = std::stod(amountStr); } catch(...) { return; }

    if (selection == "1") {
        wallet.insertCurrency(currency, amount);
        logTransaction("deposit", currency + "/FLAT", 1.0, amount);
        syncWalletFile();
        std::cout << std::endl;
        std::cout << "Deposited successfully!" << std::endl;
        std::cout << std::endl;
    } 
    else if (selection == "2") {
        if (wallet.removeCurrency(currency, amount)) {
            logTransaction("withdraw", currency + "/FLAT", 1.0, amount);
            syncWalletFile();
            std::cout << std::endl;
            std::cout << "Withdrawn successfully!" << std::endl;
            std::cout << std::endl;
        } else {
            std::cout << std::endl;
            std::cout << "Transaction Failed: Insufficient funds or non-existent token balance." << std::endl;
            std::cout << std::endl;
        }
    }
}

void MerkelMain::showRecentTransactions()
{
    std::cout << "RECENT TRADING TRANSACTIONS\n" << std::endl;
    std::vector<TransactionRecord> recent = db.getRecentTransactions(currentUserProfile.username, 5);

    if (recent.empty()) {
        std::cout << "No recent operations found for your ID." << std::endl;
        return;
    }

    std::cout << "Timestamp\t\tProduct\t\tAction\t\tPrice\t\tAmount\t\tTotal Value" << std::endl;
    for (const auto& tx : recent) {
        std::cout << tx.timestamp << "\t" << tx.product << "\t" << tx.type << "\t\t"
                  << tx.price << "\t\t" << tx.amount << "\t\t" << tx.total << std::endl;
    }
    std::cout << std::endl;
}

void MerkelMain::showUserActivityStats()
{
    std::cout << "ACCOUNT ACTIVITY PERFORMANCE ANALYTICS" << std::endl;
    std::cout << "Enter product profile filter (or hit Enter for all products combined): ";
    std::string productFilter;
    std::getline(std::cin, productFilter);

    std::cout << "Filter by date range? (y/n): ";
    std::string filterChoice;
    std::getline(std::cin, filterChoice);
    std::string startDate = "";
    std::string endDate = "";
    if (filterChoice == "y" || filterChoice == "Y") {
        std::cout << "Enter start date (YYYY/MM/DD): ";
        std::getline(std::cin, startDate);
        std::cout << "Enter end date (YYYY/MM/DD): ";
        std::getline(std::cin, endDate);
    }

    std::vector<TransactionRecord> txs = db.getTransactionsForUser(
        currentUserProfile.username, productFilter, startDate, endDate);

    int askSalesCount = 0;
    int bidSalesCount = 0;
    double totalMoneySpent = 0.0;

    for (const auto& tx : txs) {
        if (tx.type == "asksale") {
            askSalesCount++;
        } else if (tx.type == "bidsale") {
            bidSalesCount++;
            totalMoneySpent += tx.total;
        }
    }

    std::cout << "\nMetrics Summary: " << (productFilter.empty() ? "All Products" : productFilter)
               << " | " << (startDate.empty() && endDate.empty() ? "All Time" : (startDate + " to " + endDate)) << std::endl;
    std::cout << "Successful Ask Sales Executed: " << askSalesCount << std::endl;
    std::cout << "Successful Bid Purchases Executed: " << bidSalesCount << std::endl;
    std::cout << "Total Base Capital Expended (Bids): " << totalMoneySpent << std::endl;
    std::cout << std::endl;
}

void MerkelMain::runTradingSimulation()
{
    std::cout << "RUNNING RUNTIME TRADING SIMULATION ENGINE (2026)" << std::endl;

    std::vector<std::string> products = orderBook.getKnownProducts();
    std::string simTime = getSystemTimestamp();
    
    int transactionsGenerated = 0;

    // Seed randomness
    std::srand(std::time(0));

    for (const std::string& prod : products) {
        std::vector<std::string> tokens = CSVReader::tokenise(prod, '/');
        if (tokens.size() < 2) continue;
        std::string baseCurrency = tokens[0];  // e.g. ETH
        std::string quoteCurrency = tokens[1]; // e.g. BTC

        // Get live baseline data for price bounds
        std::vector<OrderBookEntry> historicalAsks = orderBook.getOrders(OrderBookType::ask, prod, orderBook.getEarliestTime());
        std::vector<OrderBookEntry> historicalBids = orderBook.getOrders(OrderBookType::bid, prod, orderBook.getEarliestTime());
        
        double lowPrice = OrderBook::getLowPrice(historicalAsks);
        double highPrice = OrderBook::getHighPrice(historicalAsks);
        
        // Fallback safety bounds if empty
        if (lowPrice == 0.0) lowPrice = 10.0;
        if (highPrice == 0.0) highPrice = 100.0;

        // 1. Generate 5 simulated BID orders (User buying baseCurrency using quoteCurrency)
        for (int i = 0; i < 5; ++i) {
            // Generate random price within baseline bounds
            double factor = (double)(std::rand() % 100) / 100.0;
            double simulatedPrice = lowPrice + (factor * (highPrice - lowPrice));
            double simulatedAmount = 0.1 + ((double)(std::rand() % 10) / 5.0); // Random amount
            double totalCost = simulatedPrice * simulatedAmount;

            // Insufficient wallet check guard clause
            if (!wallet.containsCurrency(quoteCurrency, totalCost)) {
                std::cout << "[SIM SKIPPED] Insufficient " << quoteCurrency << " to place Bid on " << prod << std::endl;
                continue;
            }

            // Create OrderBookEntry simulating standard execution context
            OrderBookEntry bidOrder{simulatedPrice, simulatedAmount, simTime, prod, OrderBookType::bidsale, currentUserProfile.username};
            
            // Execute trade adjustments live
            wallet.processSale(bidOrder);
            orderBook.insertOrder(bidOrder);
            logTransaction("bidsale", prod, simulatedPrice, simulatedAmount);
            transactionsGenerated++;
        }

        // 2. Generate 5 simulated ASK orders (User selling baseCurrency for quoteCurrency)
        for (int i = 0; i < 5; ++i) {
            double factor = (double)(std::rand() % 100) / 100.0;
            double simulatedPrice = lowPrice + (factor * (highPrice - lowPrice));
            double simulatedAmount = 0.1 + ((double)(std::rand() % 10) / 5.0);

            // Insufficient wallet check guard clause
            if (!wallet.containsCurrency(baseCurrency, simulatedAmount)) {
                std::cout << "[SIM SKIPPED] Insufficient " << baseCurrency << " to place Ask on " << prod << std::endl;
                continue;
            }

            OrderBookEntry askOrder{simulatedPrice, simulatedAmount, simTime, prod, OrderBookType::asksale, currentUserProfile.username};
            
            wallet.processSale(askOrder);
            orderBook.insertOrder(askOrder);
            logTransaction("asksale", prod, simulatedPrice, simulatedAmount);
            transactionsGenerated++;
        }
    }

    // Save final snapshots out to persistent tracking text databases
    syncWalletFile();

    std::cout << "\nSimulation completed successfully!" << std::endl;
    std::cout << "\nSuccessfully processed and logged " << transactionsGenerated << " new 2026 transactions." << std::endl;
    std::cout << "Wallet data persistent snapshots synchronized.\n" << std::endl;
}