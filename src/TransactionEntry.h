#pragma once
#include <string>

// compact representation of a persisted transaction ledger row
struct TransactionEntry {
    std::string username;
    std::string timestamp;
    std::string product;
    std::string type; // deposit/withdraw/asksale/bidsale
    double price;
    double amount;
    double totalValue; // price * amount or raw amount for flats
};