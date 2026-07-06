#pragma once
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include "OHLCEntry.h"
#include <string>
#include <vector>

class OrderBook
{
public:
    // construct by loading a CSV order file
    OrderBook(std::string filename);
    // return list of known products in dataset
    std::vector<std::string> getKnownProducts();
    // return orders filtered by type/product/timestamp
    std::vector<OrderBookEntry> getOrders(OrderBookType type,
                                          std::string product,
                                          std::string timestamp);

    // return earliest timestamp present in the orderbook
    std::string getEarliestTime();
    // return the next timestamp after `timestamp`; wrap to start if none
    std::string getNextTime(std::string timestamp);

    void insertOrder(OrderBookEntry &order);

    std::vector<OrderBookEntry> matchAsksToBids(std::string product, std::string timestamp);

    static double getHighPrice(std::vector<OrderBookEntry> &orders);
    static double getLowPrice(std::vector<OrderBookEntry> &orders);

    // return per-day OHLC stats for a product and type (optional date filter)
    std::vector<OHLCEntry> getOHLC(OrderBookType type, 
                                   std::string product, 
                                   std::string startDate = "", 
                                   std::string endDate = "");

private:
    std::vector<OrderBookEntry> orders;
};
