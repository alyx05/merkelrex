#pragma once
#include <string>

struct TransactionEntry {
    std::string username;
    std::string timestamp;
    std::string product;
    std::string type; // "deposit", "withdraw", "asksale", "bidsale"
    double price;
    double amount;
    double totalValue; // price * amount (or just amount for simple flat adjustments)
};