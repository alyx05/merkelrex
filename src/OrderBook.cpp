#include "OrderBook.h"
#include "CSVReader.h"
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>

// construct and populate orders from CSV files
OrderBook::OrderBook(std::string filename)
{
    // load historical market data
    orders = CSVReader::readCSV(filename);

    // load any persisted simulation trades and merge into `orders`
    std::ifstream file("src/USERS_TRADING.CSV");
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            std::vector<std::string> tokens = CSVReader::tokenise(line, ',');
            if (tokens.size() >= 7)
            {
                std::string timestamp = tokens[1];
                std::string product = tokens[2];
                std::string typeStr = tokens[3];
                double price = std::stod(tokens[4]);
                double amount = std::stod(tokens[5]);
                std::string username = tokens[0];

                OrderBookType type = OrderBookType::unknown;
                if (typeStr == "asksale") type = OrderBookType::asksale;
                if (typeStr == "bidsale") type = OrderBookType::bidsale;

                if (type != OrderBookType::unknown)
                {
                    OrderBookEntry obe{price, amount, timestamp, product, type, username};
                    orders.push_back(obe);
                }
            }
        }
        // keep orders chronologically sorted
        std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
    }
}

/** return vector of all know products in the dataset*/
std::vector<std::string> OrderBook::getKnownProducts()
{
    std::vector<std::string> products;

    std::map<std::string, bool> prodMap;

    for (OrderBookEntry &e : orders)
    {
        prodMap[e.product] = true;
    }

    // now flatten the map to a vector of strings
    for (auto const &e : prodMap)
    {
        products.push_back(e.first);
    }

    return products;
}
/** return vector of Orders according to the sent filters*/
std::vector<OrderBookEntry> OrderBook::getOrders(OrderBookType type,
                                                 std::string product,
                                                 std::string timestamp)
{
    std::vector<OrderBookEntry> orders_sub;
    for (OrderBookEntry &e : orders)
    {
        if (e.orderType == type &&
            e.product == product &&
            e.timestamp == timestamp)
        {
            orders_sub.push_back(e);
        }
    }
    return orders_sub;
}

double OrderBook::getHighPrice(std::vector<OrderBookEntry> &orders)
{
    if (orders.empty()) return 0.0;
    double max = orders[0].price;
    for (OrderBookEntry &e : orders)
    {
        if (e.price > max)
            max = e.price;
    }
    return max;
}

double OrderBook::getLowPrice(std::vector<OrderBookEntry> &orders)
{
    if (orders.empty()) return 0.0;
    double min = orders[0].price;
    for (OrderBookEntry &e : orders)
    {
        if (e.price < min)
            min = e.price;
    }
    return min;
}

std::string OrderBook::getEarliestTime()
{
    return orders[0].timestamp;
}

std::string OrderBook::getNextTime(std::string timestamp)
{
    std::string next_timestamp = "";
    for (OrderBookEntry &e : orders)
    {
        if (e.timestamp > timestamp)
        {
            next_timestamp = e.timestamp;
            break;
        }
    }
    if (next_timestamp == "")
    {
        next_timestamp = orders[0].timestamp;
    }
    return next_timestamp;
}

void OrderBook::insertOrder(OrderBookEntry &order)
{
    orders.push_back(order);
    // Note: this re-sorts the full order vector on every insert,
    // which can become expensive on large datasets.
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}

std::vector<OrderBookEntry> OrderBook::matchAsksToBids(std::string product, std::string timestamp)
{
    // collect asks at timestamp
    std::vector<OrderBookEntry> asks = getOrders(OrderBookType::ask,
                                                 product,
                                                 timestamp);
    // collect bids at timestamp
    std::vector<OrderBookEntry> bids = getOrders(OrderBookType::bid,
                                                 product,
                                                 timestamp);
    // prepare sales vector
    std::vector<OrderBookEntry> sales;

    // return empty if either side has no orders
    if (asks.size() == 0 || bids.size() == 0)
    {
        std::cout << "No bids or asks." << std::endl;
        std::cout << std::endl;
        return sales;
    }
    // sort asks lowest first, bids highest first
    std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
    // sort bids highest first
    std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);
    std::cout << "Max Ask: " << asks[asks.size() - 1].price << std::endl;
    std::cout << "Min Ask: " << asks[0].price << std::endl;
    std::cout << "Max Bid: " << bids[0].price << std::endl;
    std::cout << "Min Bid: " << bids[bids.size() - 1].price << std::endl;

    for (OrderBookEntry &ask : asks)
    {
        // iterate bids to match this ask
        for (OrderBookEntry &bid : bids)
        {
            // match when bid price >= ask price
            if (bid.price >= ask.price)
            {
                // create sale at ask price
                OrderBookEntry sale{ask.price, 0, timestamp,
                                    product,
                                    OrderBookType::asksale};

                if (bid.username == "simuser")
                {
                    sale.username = "simuser";
                    sale.orderType = OrderBookType::bidsale;
                }
                if (ask.username == "simuser")
                {
                    sale.username = "simuser";
                    sale.orderType = OrderBookType::asksale;
                }

                // determine matched amounts and adjust orders
                // case: bid.amount == ask.amount -> full match
                if (bid.amount == ask.amount)
                {
                    // sale amount = ask amount
                    sale.amount = ask.amount;
                    // record sale
                    sales.push_back(sale);
                    // mark bid consumed
                    bid.amount = 0;
                    // move to next ask
                    break;
                }
                // case: bid.amount > ask.amount -> bid partially fills
                if (bid.amount > ask.amount)
                {
                    // sale amount = ask amount
                    sale.amount = ask.amount;
                    // record sale
                    sales.push_back(sale);
                    // reduce bid amount for further matching
                    bid.amount = bid.amount - ask.amount;
                    // move to next ask
                    break;
                }
                // case: bid.amount < ask.amount -> bid consumed, ask partially remains
                if (bid.amount < ask.amount &&
                    bid.amount > 0)
                {
                    // sale amount = bid amount
                    sale.amount = bid.amount;
                    // record sale
                    sales.push_back(sale);
                    // decrease ask amount by consumed bid
                    ask.amount = ask.amount - bid.amount;
                    // mark bid consumed
                    bid.amount = 0;
                    // continue matching this ask with next bids
                    continue;
                }
            }
        }
    }
    return sales;
}

#include <map>

// extract date part
std::string extractDate(const std::string& timestamp) {
    if (timestamp.length() >= 10) {
        return timestamp.substr(0, 10);
    }
    return timestamp;
}

std::vector<OHLCEntry> OrderBook::getOHLC(OrderBookType type, 
                                          std::string product, 
                                          std::string startDate, 
                                          std::string endDate)
{
    std::vector<OHLCEntry> ohlcList;
    
    std::vector<OrderBookEntry> filteredOrders;
    for (const OrderBookEntry& e : orders) {

        bool typeMatches = (e.orderType == type) || 
                       (type == OrderBookType::ask && e.orderType == OrderBookType::asksale) ||
                       (type == OrderBookType::bid && e.orderType == OrderBookType::bidsale);

        if (typeMatches && e.product == product) {
            std::string orderDate = extractDate(e.timestamp);
            
            // date filters (if yes)
            if (!startDate.empty() && orderDate < startDate) continue;
            if (!endDate.empty() && orderDate > endDate) continue;
            
            filteredOrders.push_back(e);
        }
    }
    
    if (filteredOrders.empty()) return ohlcList;

    // sort by timestamp
    std::sort(filteredOrders.begin(), filteredOrders.end(), OrderBookEntry::compareByTimestamp);

    // group by date
    std::map<std::string, std::vector<OrderBookEntry>> ordersByDate;
    for (const auto& e : filteredOrders) {
        ordersByDate[extractDate(e.timestamp)].push_back(e);
    }

    // calculate per day ohlc stats
    for (const auto& constPair : ordersByDate) {
        const std::string& date = constPair.first;
        const std::vector<OrderBookEntry>& dayOrders = constPair.second;

        double openPrice = dayOrders.front().price;
        double closePrice = dayOrders.back().price;
        double highPrice = dayOrders[0].price;
        double lowPrice = dayOrders[0].price;

        for (const auto& e : dayOrders) {
            if (e.price > highPrice) highPrice = e.price;
            if (e.price < lowPrice) lowPrice = e.price;
        }

        ohlcList.push_back({date, openPrice, highPrice, lowPrice, closePrice});
    }

    return ohlcList;
}