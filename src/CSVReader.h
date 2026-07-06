#pragma once

#include "OrderBookEntry.h"
#include <vector>
#include <string>

class CSVReader
{
public:
    CSVReader();

    // read CSV file into OrderBookEntry vector
    static std::vector<OrderBookEntry> readCSV(std::string csvFile);
    // split a string by separator into tokens
    static std::vector<std::string> tokenise(std::string csvLine, char separator);

    // helper to build an OrderBookEntry from discrete fields
    static OrderBookEntry stringsToOBE(std::string price,
                                       std::string amount,
                                       std::string timestamp,
                                       std::string product,
                                       OrderBookType OrderBookType);

private:
    static OrderBookEntry stringsToOBE(std::vector<std::string> strings);
};