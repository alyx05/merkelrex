#pragma once
#include <string>

// one-day OHLC record used for per-day statistics
struct OHLCEntry
{
    std::string date;
    double open;
    double high;
    double low;
    double close;
};