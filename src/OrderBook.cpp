// orderbook.cpp

#include "OrderBook.h"
#include "CSVReader.h"
#include <map>
#include <algorithm>
#include <iostream>
#include <fstream>

// construct and populate orders from CSV files
OrderBook::OrderBook(std::string filename, Database& database)
{
    orders = CSVReader::readCSV(filename);
    if (orders.empty())
    {
        throw std::runtime_error("OrderBook: failed to load any data from '" + filename +
                                  "' (check the file exists and the path is correct)");
    }

    // pull persisted simulation trades from SQLite instead of parsing
    // USERS_TRADING.CSV directly, and merge into `orders`
    std::vector<OrderBookEntry> saleHistory = database.getAllSaleEntries();
    orders.insert(orders.end(), saleHistory.begin(), saleHistory.end());

    // keep orders chronologically sorted
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
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

std::vector<OrderBookEntry> OrderBook::getOrdersByProduct(OrderBookType type, std::string product)
{
    std::vector<OrderBookEntry> result;
    for (OrderBookEntry &e : orders)
    {
        if (e.orderType == type && e.product == product)
        {
            result.push_back(e);
        }
    }
    return result;
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
    if (orders.empty())
        throw std::runtime_error("OrderBook::getEarliestTime called with no orders loaded");
    return orders[0].timestamp;}

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
    std::vector<OrderBookEntry> asks = getOrders(OrderBookType::ask, product, timestamp);
    std::vector<OrderBookEntry> bids = getOrders(OrderBookType::bid, product, timestamp);
    std::vector<OrderBookEntry> sales;

    if (asks.empty() || bids.empty())
    {
        std::cout << "No bids or asks." << std::endl << std::endl;
        return sales;
    }

    // Price-time priority books: best price is always at begin().
    std::map<double, std::queue<OrderBookEntry>, std::less<double>> askBook;    // lowest ask first
    std::map<double, std::queue<OrderBookEntry>, std::greater<double>> bidBook; // highest bid first

    for (OrderBookEntry &a : asks) askBook[a.price].push(a);
    for (OrderBookEntry &b : bids) bidBook[b.price].push(b);

    std::cout << "Max Ask: " << askBook.rbegin()->first << std::endl;
    std::cout << "Min Ask: " << askBook.begin()->first << std::endl;
    std::cout << "Max Bid: " << bidBook.begin()->first << std::endl;
    std::cout << "Min Bid: " << bidBook.rbegin()->first << std::endl;

    while (!askBook.empty() && !bidBook.empty())
    {
        auto askLevel = askBook.begin();  // O(1) best ask
        auto bidLevel = bidBook.begin();  // O(1) best bid

        if (bidLevel->first < askLevel->first)
            break; // no crossing prices left, matching is done

        OrderBookEntry &ask = askLevel->second.front();
        OrderBookEntry &bid = bidLevel->second.front();

        OrderBookEntry sale{ask.price, 0, timestamp, product, OrderBookType::asksale};

        // old:
        // if (bid.username == "simuser") { sale.username = "simuser"; sale.orderType = OrderBookType::bidsale; }
        // if (ask.username == "simuser") { sale.username = "simuser"; sale.orderType = OrderBookType::asksale; }

        // new: attribute the sale to whichever side is a real placed order (i.e. not
        // "dataset", the default for historical CSV rows). If both sides happen to
        // be real, distinct users (two API-placed orders crossing each other), the
        // ask side wins here and the bid side's settlement is a known gap -- see
        // the settleSale rollout notes for the planned follow-up fix.
        if (bid.username != "dataset")
        {
            sale.username = bid.username;
            sale.orderType = OrderBookType::bidsale;
        }
        if (ask.username != "dataset")
        {
            sale.username = ask.username;
            sale.orderType = OrderBookType::asksale;
        }

        if (bid.amount == ask.amount)
        {
            sale.amount = ask.amount;
            sales.push_back(sale);
            askLevel->second.pop();
            bidLevel->second.pop();
        }
        else if (bid.amount > ask.amount)
        {
            sale.amount = ask.amount;
            sales.push_back(sale);
            bid.amount -= ask.amount;
            askLevel->second.pop();
        }
        else // bid.amount < ask.amount
        {
            sale.amount = bid.amount;
            sales.push_back(sale);
            ask.amount -= bid.amount;
            bidLevel->second.pop();
        }

        if (askLevel->second.empty()) askBook.erase(askLevel);
        if (bidLevel->second.empty()) bidBook.erase(bidLevel);
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

std::vector<OrderBookEntry> OrderBook::matchNewOrder(OrderBookEntry& newOrder)
{
    std::vector<OrderBookEntry> sales;
    OrderBookType oppositeType = (newOrder.orderType == OrderBookType::ask)
        ? OrderBookType::bid : OrderBookType::ask;

    // find resting opposite-side orders for this product, best price first
    std::vector<size_t> oppIdx;
    for (size_t i = 0; i < orders.size(); ++i)
    {
        if (orders[i].product == newOrder.product && orders[i].orderType == oppositeType)
            oppIdx.push_back(i);
    }
    if (oppIdx.empty()) return sales;

    if (newOrder.orderType == OrderBookType::ask)
    {
        // matching an ask: want the highest-price resting bids first
        std::sort(oppIdx.begin(), oppIdx.end(), [this](size_t a, size_t b) { return orders[a].price > orders[b].price; });
    }
    else
    {
        // matching a bid: want the lowest-price resting asks first
        std::sort(oppIdx.begin(), oppIdx.end(), [this](size_t a, size_t b) { return orders[a].price < orders[b].price; });
    }

    std::vector<bool> consumed(orders.size(), false);
    const size_t MAX_FILLS_PER_CALL = 50;

    for (size_t idx : oppIdx)
    {
        if (sales.size() >= MAX_FILLS_PER_CALL) break;
        if (newOrder.amount <= 0) break;

        OrderBookEntry& counter = orders[idx];

        bool crosses = (newOrder.orderType == OrderBookType::ask)
            ? (counter.price >= newOrder.price)
            : (counter.price <= newOrder.price);
        if (!crosses) break;

        OrderBookEntry& ask = (newOrder.orderType == OrderBookType::ask) ? newOrder : counter;
        OrderBookEntry& bid = (newOrder.orderType == OrderBookType::bid) ? newOrder : counter;

        double fillAmount = std::min(newOrder.amount, counter.amount);

        OrderBookEntry askSale{counter.price, fillAmount, newOrder.timestamp, newOrder.product, OrderBookType::asksale, ask.username};
        OrderBookEntry bidSale{counter.price, fillAmount, newOrder.timestamp, newOrder.product, OrderBookType::bidsale, bid.username};

        if (ask.username != "dataset") sales.push_back(askSale);
        if (bid.username != "dataset") sales.push_back(bidSale);

        newOrder.amount -= fillAmount;
        counter.amount -= fillAmount;
        if (counter.amount <= 0) consumed[idx] = true;
    }

    if (!consumed.empty())
    {
        std::vector<OrderBookEntry> remaining;
        remaining.reserve(orders.size());
        for (size_t i = 0; i < orders.size(); ++i)
        {
            if (!consumed[i]) remaining.push_back(std::move(orders[i]));
        }
        orders = std::move(remaining);
    }

    return sales;
}