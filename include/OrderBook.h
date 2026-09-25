// orderbook.h

#pragma once
#include "OrderBookEntry.h"
#include "CSVReader.h"
#include "OHLCEntry.h"
#include "Database.h"
#include <string>
#include <vector>
#include <map>
#include <queue>

class OrderBook
{
public:
    // construct by loading a CSV order file
    OrderBook(std::string filename, Database& database);    // return list of known products in dataset
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

    // return all currently-resting orders of a given type for a product,
    // regardless of timestamp -- used by the API's "live depth" endpoint.
    // Note: passing OrderBookType::bid or ::ask here naturally excludes
    // already-matched bidsale/asksale entries, since those are a different enum value.
    std::vector<OrderBookEntry> getOrdersByProduct(OrderBookType type, std::string product);

    // Match a single newly-placed order against the opposite side of the book
    // for its product, ignoring timestamp. Fills the order against the best
    // available opposing price(s) until it's fully filled or no crossing
    // counterparty remains, then leaves any unfilled remainder resting in the
    // book. This targets the specific triggering order, rather than attempting
    // to clear the entire book's backlog on every call (see rollout notes: the
    // historical dataset is not a cleanly-matched book, so a whole-book sweep
    // can never reach a newly placed order if enough historical crossings sit
    // ahead of it in price order).
    std::vector<OrderBookEntry> matchNewOrder(OrderBookEntry& newOrder);

    // return per-day OHLC stats for a product and type (optional date filter)
    std::vector<OHLCEntry> getOHLC(OrderBookType type, 
                                   std::string product, 
                                   std::string startDate = "", 
                                   std::string endDate = "");

private:
    std::vector<OrderBookEntry> orders;
};
