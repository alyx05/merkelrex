#pragma once

#include <string>
#include <map>
#include "OrderBookEntry.h"
#include <iostream>

class Wallet 
{
    public:
        Wallet();
        // add amount to a currency balance
        void insertCurrency(std::string type, double amount);
        // subtract amount from a currency balance
        bool removeCurrency(std::string type, double amount);
        // check if balance >= amount
        bool containsCurrency(std::string type, double amount);
        // check if this order can be funded from the wallet
        bool canFulfillOrder(OrderBookEntry order);
        // apply a completed sale to wallet balances (assumes owner)
        void processSale(OrderBookEntry& sale);

        // return a human-readable wallet string
        std::string toString();
        friend std::ostream& operator<<(std::ostream& os, Wallet& wallet);

        // return copy of balances for persistence
        std::map<std::string,double> getBalances() const;

        
    private:
        std::map<std::string,double> currencies;

};