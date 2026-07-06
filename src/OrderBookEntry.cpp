#include "OrderBookEntry.h"

// construct simple POD order entry
OrderBookEntry::OrderBookEntry(double _price,
                               double _amount,
                               std::string _timestamp,
                               std::string _product,
                               OrderBookType _orderType,
                               std::string _username)
    : price(_price),
      amount(_amount),
      timestamp(_timestamp),
      product(_product),
      orderType(_orderType),
      username(_username)
{
}

// convert token to OrderBookType enum
OrderBookType OrderBookEntry::stringToOrderBookType(std::string s)
{
  if (s == "ask") return OrderBookType::ask;
  if (s == "bid") return OrderBookType::bid;
  return OrderBookType::unknown;
}
