#include <iostream>
#include <vector>
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
    int input;
    currentTime = orderBook.getEarliestTime();

    wallet.insertCurrency("BTC", 10);

// Enforce Authentication Loop first
    while (!isLoggedIn) {
        showAuthMenu();
        std::string choiceLine;
        std::getline(std::cin, choiceLine);
        int authChoice = 0;
        try { authChoice = std::stoi(choiceLine); } catch(...) {}

        if (authChoice == 1) handleLogin();
        else if (authChoice == 2) handleRegister();
        else if (authChoice == 3) handlePasswordReset();
        else if (authChoice == 4) {
            std::cout << "Exiting system. Goodbye!" << std::endl;
            return;
        } else {
            std::cout << "Invalid choice. Please pick 1-4." << std::endl;
        }
    }

    while (true)
    {
        printMenu();
        input = getUserOption();
        processUserOption(input);
    }
}

void MerkelMain::printMenu()
{
    // 1 print help
    std::cout << "1: Print help " << std::endl;
    // 2 print exchange stats
    std::cout << "2: Print exchange stats" << std::endl;
    // 3 make an offer
    std::cout << "3: Make an offer " << std::endl;
    // 4 make a bid
    std::cout << "4: Make a bid " << std::endl;
    // 5 print wallet
    std::cout << "5: Print wallet " << std::endl;
    // 6 continue
    std::cout << "6: Continue " << std::endl;
    // 7 view ohlc stats
    std::cout << "7: View Product OHLC Data " << std::endl;
    // 8 fund transactions
    std::cout << "8: Deposit or Withdraw Funds" << std::endl;
    // 9 view transactions
    std::cout << "9: View Recent Transactions" << std::endl;
    // 10 view summary
    std::cout << "10: View Activity Summary Statistics" << std::endl;
    // 11 view activity
    std::cout << "11: Run 2026 Trading Activity Simulator" << std::endl;

    std::cout << "============== " << std::endl;

    std::cout << "Current time is: " << currentTime << std::endl;
}

int MerkelMain::getUserOption()
{
    int userOption = 0;
    std::string line;
    std::cout << "Type in 1-11" << std::endl;
    std::getline(std::cin, line);
    try
    {
        userOption = std::stoi(line);
    }
    catch (const std::exception &e)
    {
        //
    }
    std::cout << "You chose: " << userOption << std::endl;
    return userOption;
}

void MerkelMain::processUserOption(int userOption)
{
    if (userOption == 0) // bad input
    {
        std::cout << "Invalid choice. Choose 1-6" << std::endl;
    }
    if (userOption == 1)
    {
        printHelp();
    }
    if (userOption == 2)
    {
        printMarketStats();
    }
    if (userOption == 3)
    {
        enterAsk();
    }
    if (userOption == 4)
    {
        enterBid();
    }
    if (userOption == 5)
    {
        printWallet();
    }
    if (userOption == 6)
    {
        gotoNextTimeframe();
    }
    if (userOption == 7)
    {
        handleOHLC();
    }
    if (userOption == 8)
    {
        handleWalletAdjustments();
    }
    if (userOption == 9)
    {
        showRecentTransactions();
    }
    if (userOption == 10)
    {
        showUserActivityStats();
    }
    if (userOption == 11)
    {
        runTradingSimulation();
    }
}

void MerkelMain::showAuthMenu()
{
    std::cout << "=================================" << std::endl;
    std::cout << " WELCOME TO MERKLEREX TRADING   " << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "1: Log In" << std::endl;
    std::cout << "2: Register New Profile" << std::endl;
    std::cout << "3: Reset Password" << std::endl;
    std::cout << "4: Exit" << std::endl;
    std::cout << "Choose an option: ";
}

// Check if a name/email combo is already registered
bool MerkelMain::userExists(const std::string& fullName, const std::string& email)
{
    std::ifstream file("src/USERS_REGISTER.CSV");
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() >= 3) {
            // tokens[1] is fullName, tokens[2] is email
            if (tokens[1] == fullName && tokens[2] == email) {
                return true;
            }
        }
    }
    return false;
}

// Read and load a single profile by username
bool MerkelMain::loadUser(const std::string& username, User& outUser)
{
    std::ifstream file("src/USERS_REGISTER.CSV");
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() >= 4 && tokens[0] == username) {
            outUser.username = tokens[0];
            outUser.fullName = tokens[1];
            outUser.email = tokens[2];
            outUser.passwordHash = std::stoull(tokens[3]); // Convert string to size_t
            return true;
        }
    }
    return false;
}

// Append new registration profile to file
void MerkelMain::saveUserToFile(const User& user)
{
    std::ofstream file("src/USERS_REGISTER.CSV", std::ios::app);
    if (file.is_open()) {
        file << user.username << "," 
             << user.fullName << "," 
             << user.email << "," 
             << user.passwordHash << "\n";
    }
}

// Write the sign-up bonus to USERS_WALLET.CSV
void MerkelMain::saveInitialWallet(const std::string& username, double bonusAmount)
{
    std::ofstream file("src/USERS_WALLET.CSV", std::ios::app);
    if (file.is_open()) {
        file << username << "," << bonusAmount << "\n";
    }
}

void MerkelMain::handleRegister()
{
    std::string fullName, email, password;
    std::cout << "\n=== REGISTER NEW ACCOUNT ===" << std::endl;
    std::cout << "Enter Full Name: ";
    std::getline(std::cin, fullName);
    std::cout << "Enter Email Address: ";
    std::getline(std::cin, email);

    if (userExists(fullName, email)) {
        std::cout << "Error: An account with this name and email already exists!\n" << std::endl;
        return;
    }

    std::cout << "Enter Password: ";
    std::getline(std::cin, password);

    // 1. Generate 10-digit unique username
    std::srand(std::time(0));
    std::string newUsername = "";
    for (int i = 0; i < 10; ++i) {
        newUsername += std::to_string(std::rand() % 10);
    }

    // 2. Hash Password
    std::hash<std::string> stringHasher;
    size_t hashedPass = stringHasher(password);

    // 3. Create and Save User Profile
    User newUser{newUsername, fullName, email, hashedPass};
    saveUserToFile(newUser);

    // 4. Record sign up bonus in wallet database (e.g., 5000 USDT simulation bonus)
    double signUpBonus = 5000.0;
    saveInitialWallet(newUsername, signUpBonus);
    
    // Dynamically give active wallet context some start capital too!
    wallet.insertCurrency("USDT", signUpBonus);

    std::cout << "\nRegistration Successful!" << std::endl;
    std::cout << ">> YOUR UNIQUE LOGIN USERNAME IS: " << newUsername << " <<" << std::endl;
    std::cout << "Please write this down! You will need it to login.\n" << std::endl;
}

bool MerkelMain::handleLogin()
{
    std::string inputUser, inputPass;
    std::cout << "\n=== LOGIN ===" << std::endl;
    std::cout << "Enter 10-digit Username: ";
    std::getline(std::cin, inputUser);
    std::cout << "Enter Password: ";
    std::getline(std::cin, inputPass);

    User targetUser;
    if (loadUser(inputUser, targetUser)) {
        std::hash<std::string> stringHasher;
        if (stringHasher(inputPass) == targetUser.passwordHash) {
            isLoggedIn = true;
            currentUserProfile = targetUser;
            std::cout << "\nWelcome back, " << targetUser.fullName << "! Login successful.\n" << std::endl;
            return true;
        }
    }

    std::cout << "Invalid username or password.\n" << std::endl;
    return false;
}

void MerkelMain::handlePasswordReset()
{
    std::string inputUser, inputEmail;
    std::cout << "\n=== PASSWORD RESET ===" << std::endl;
    std::cout << "Enter your 10-digit Username: ";
    std::getline(std::cin, inputUser);
    std::cout << "Confirm your Email Address: ";
    std::getline(std::cin, inputEmail);

    User targetUser;
    if (loadUser(inputUser, targetUser) && targetUser.email == inputEmail) {
        std::string newPassword;
        std::cout << "Identity verified! Enter new password: ";
        std::getline(std::cin, newPassword);

        std::hash<std::string> stringHasher;
        size_t newHash = stringHasher(newPassword);

        // Rewrite entire user register file updating this user's entry
        std::ifstream inFile("src/USERS_REGISTER.CSV");
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(inFile, line)) {
            std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
            if (!tokens.empty() && tokens[0] == inputUser) {
                // Construct updated line
                lines.push_back(targetUser.username + "," + targetUser.fullName + "," + targetUser.email + "," + std::to_string(newHash));
            } else {
                lines.push_back(line);
            }
        }
        inFile.close();

        std::ofstream outFile("src/USERS_REGISTER.CSV");
        for (const auto& l : lines) {
            outFile << l << "\n";
        }
        std::cout << "Password successfully updated! You can now log in.\n" << std::endl;
    } else {
        std::cout << "Error: Username and email combination mismatch.\n" << std::endl;
    }
}

void MerkelMain::printHelp()
{
    std::cout << "Help - your aim is to make money. Analyse the market and make bids and offers. " << std::endl;
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
    }
    // std::cout << "OrderBook contains :  " << orders.size() << " entries" << std::endl;
    // unsigned int bids = 0;
    // unsigned int asks = 0;
    // for (OrderBookEntry& e : orders)
    // {
    //     if (e.orderType == OrderBookType::ask)
    //     {
    //         asks ++;
    //     }
    //     if (e.orderType == OrderBookType::bid)
    //     {
    //         bids ++;
    //     }
    // }
    // std::cout << "OrderBook asks:  " << asks << " bids:" << bids << std::endl;
}

void MerkelMain::enterAsk()
{
    std::cout << "Make an ask - enter the amount: product,price, amount, eg  ETH/BTC,200,0.5" << std::endl;
    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if (tokens.size() != 3)
    {
        std::cout << "MerkelMain::enterAsk Bad input! " << input << std::endl;
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
            obe.username = "simuser";
            if (wallet.canFulfillOrder(obe))
            {
                std::cout << "Wallet looks good. " << std::endl;
                orderBook.insertOrder(obe);
            }
            else
            {
                std::cout << "Wallet has insufficient funds . " << std::endl;
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
    std::cout << "Make an bid - enter the amount: product,price, amount, eg  ETH/BTC,200,0.5" << std::endl;
    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> tokens = CSVReader::tokenise(input, ',');
    if (tokens.size() != 3)
    {
        std::cout << "MerkelMain::enterBid Bad input! " << input << std::endl;
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
            obe.username = "simuser";

            if (wallet.canFulfillOrder(obe))
            {
                std::cout << "Wallet looks good. " << std::endl;
                orderBook.insertOrder(obe);
            }
            else
            {
                std::cout << "Wallet has insufficient funds . " << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << " MerkelMain::enterBid Bad input " << std::endl;
        }
    }
}

void MerkelMain::printWallet()
{
    std::cout << wallet.toString() << std::endl;
}

void MerkelMain::gotoNextTimeframe()
{
    std::cout << "Going to next time frame. " << std::endl;
    for (std::string p : orderBook.getKnownProducts())
    {
        std::cout << "matching " << p << std::endl;
        std::vector<OrderBookEntry> sales = orderBook.matchAsksToBids(p, currentTime);
        std::cout << "Sales: " << sales.size() << std::endl;
        for (OrderBookEntry &sale : sales)
        {
            std::cout << "Sale price: " << sale.price << " amount " << sale.amount << std::endl;
            if (sale.username == "simuser")
            {
                // update the wallet
                wallet.processSale(sale);

                std::string actionType = (sale.orderType == OrderBookType::asksale) ? "asksale" : "bidsale";
                logTransaction(actionType, sale.product, sale.price, sale.amount);
                syncWalletFile();
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

// Rewrite USERS_WALLET.CSV with fresh snapshots across ALL currencies for this user
void MerkelMain::syncWalletFile()
{
    // For a production app, we would selectively patch entries. 
    // For our scope, we rewrite/append safely by ensuring the file reflects current user states.
    std::ofstream file("src/USERS_WALLET.CSV", std::ios::app);
    if (file.is_open()) {
        // Appending latest currency state updates
        file << currentUserProfile.username << ",SYNC," << wallet.toString() << "\n";
    }
}

void MerkelMain::logTransaction(const std::string& type, const std::string& product, double price, double amount)
{
    std::ofstream file("src/USERS_TRADING.CSV", std::ios::app);
    if (file.is_open()) {
        file << currentUserProfile.username << ","
             << getSystemTimestamp() << ","
             << product << ","
             << type << ","
             << price << ","
             << amount << ","
             << (price * amount) << "\n";
    }
}

void MerkelMain::handleWalletAdjustments()
{
    std::cout << "\n=== WALLET DEPOSIT / WITHDRAWAL SIMULATOR ===" << std::endl;
    std::cout << "1: Deposit Funds\n2: Withdraw Funds\nChoose choice: ";
    std::string selection;
    std::getline(std::cin, selection);

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
        std::cout << "Deposited successfully!" << std::endl;
    } 
    else if (selection == "2") {
        if (wallet.removeCurrency(currency, amount)) {
            logTransaction("withdraw", currency + "/FLAT", 1.0, amount);
            syncWalletFile();
            std::cout << "Withdrawn successfully!" << std::endl;
        } else {
            std::cout << "Transaction Failed: Insufficient funds or non-existent token balance." << std::endl;
        }
    }
}

void MerkelMain::showRecentTransactions()
{
    std::cout << "\n================= RECENT TRADING TRANSACTIONS =================" << std::endl;
    std::ifstream file("src/USERS_TRADING.CSV");
    if (!file.is_open()) {
        std::cout << "No trading history file records found yet." << std::endl;
        return;
    }

    std::string line;
    std::vector<std::string> userTrades;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (!tokens.empty() && tokens[0] == currentUserProfile.username) {
            userTrades.push_back(line);
        }
    }

    if (userTrades.empty()) {
        std::cout << "No recent operations found for your ID." << std::endl;
        return;
    }

    // Capture up to last 5 entries
    int count = 0;
    std::cout << "Timestamp\t\tProduct\t\tAction\t\tPrice\t\tAmount\t\tTotal Value" << std::endl;
    for (int i = userTrades.size() - 1; i >= 0 && count < 5; --i, ++count) {
        std::vector<std::string> tokens = CSVReader::tokenise(userTrades[i], ',');
        std::cout << tokens[1] << "\t" << tokens[2] << "\t" << tokens[3] << "\t\t" 
                  << tokens[4] << "\t\t" << tokens[5] << "\t\t" << tokens[6] << std::endl;
    }
    std::cout << "===============================================================\n" << std::endl;
}

void MerkelMain::showUserActivityStats()
{
    std::cout << "\n=== ACCOUNT ACTIVITY PERFORMANCE ANALYTICS ===" << std::endl;
    std::cout << "Enter product profile filter (or hit Enter for all products combined): ";
    std::string productFilter;
    std::getline(std::cin, productFilter);

    std::ifstream file("src/USERS_TRADING.CSV");
    if (!file.is_open()) {
        std::cout << "No ledger records available to compile summary stats data." << std::endl;
        return;
    }

    int askSalesCount = 0;
    int bidSalesCount = 0;
    double totalMoneySpent = 0.0;

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
        if (tokens.size() >= 7 && tokens[0] == currentUserProfile.username) {
            std::string tradeProduct = tokens[2];
            std::string type = tokens[3];
            double value = std::stod(tokens[6]);

            if (!productFilter.empty() && tradeProduct != productFilter) {
                continue; 
            }

            if (type == "asksale") {
                askSalesCount++;
            } else if (type == "bidsale") {
                bidSalesCount++;
                totalMoneySpent += value; // Tracking total base currency value outlays
            }
        }
    }

    std::cout << "\nMetrics Summary: " << (productFilter.empty() ? "All Products" : productFilter) << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    std::cout << "Successful Ask Sales Executed: " << askSalesCount << std::endl;
    std::cout << "Successful Bid Purchases Executed: " << bidSalesCount << std::endl;
    std::cout << "Total Base Capital Expended (Bids): " << totalMoneySpent << std::endl;
    std::cout << "---------------------------------------\n" << std::endl;
}

void MerkelMain::runTradingSimulation()
{
    std::cout << "\n=================================================" << std::endl;
    std::cout << " RUNNING RUNTIME TRADING SIMULATION ENGINE (2026) " << std::endl;
    std::cout << "=================================================" << std::endl;

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

    std::cout << "\n>>> Simulation completed successfully! <<<" << std::endl;
    std::cout << "Successfully processed and logged " << transactionsGenerated << " new 2026 transactions." << std::endl;
    std::cout << "Wallet data persistent snapshots synchronized.\n" << std::endl;
}