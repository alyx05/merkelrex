#include "CSVReader.h"
#include <iostream>
#include <fstream>
#include <charconv>
#include <string_view>
#include <cctype>

namespace {
    bool parseDouble(std::string_view sv, double &out)
    {
        // trim whitespace to match std::stod's tolerance for leading spaces
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
            sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
            sv.remove_suffix(1);

        if (sv.empty()) return false;

        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
        return ec == std::errc() && ptr == sv.data() + sv.size(); // must consume the entire token
    }
}

std::vector<std::string> CSVReader::tokenise(std::string_view csvLine, char separator)
{
    std::vector<std::string> tokens;
    size_t start = csvLine.find_first_not_of(separator, 0);

    while (start != std::string_view::npos)
    {
        size_t end = csvLine.find_first_of(separator, start);
        std::string_view token = (end == std::string_view::npos)
            ? csvLine.substr(start)
            : csvLine.substr(start, end - start);
        tokens.emplace_back(token); // single allocation, here — not during scanning
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return tokens;
}

CSVReader::CSVReader()
{
}

std::vector<OrderBookEntry> CSVReader::readCSV(std::string csvFilename)
{
    std::vector<OrderBookEntry> entries;

    // stream lines and parse into OrderBookEntry objects
    std::ifstream csvFile{csvFilename};
    std::string line;
    if (csvFile.is_open())
    {
        while (std::getline(csvFile, line))
        {
            try
            {
                OrderBookEntry obe = stringsToOBE(tokenise(line, ','));
                entries.push_back(obe);
            }
            catch (const std::exception &e)
            {
                std::cout << "CSVReader::readCSV bad data" << std::endl;
            }
        }
    }

    return entries;
}

OrderBookEntry CSVReader::stringsToOBE(std::vector<std::string> tokens)
{
    if (tokens.size() != 5)
    {
        std::cout << "Bad line " << std::endl;
        throw std::exception{};
    }

    double price, amount;
    if (!parseDouble(tokens[3], price))
    {
        std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[3] << std::endl;
        throw std::exception{};
    }
    if (!parseDouble(tokens[4], amount))
    {
        std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[4] << std::endl;
        throw std::exception{};
    }

    return OrderBookEntry{price, amount, tokens[0], tokens[1],
                          OrderBookEntry::stringToOrderBookType(tokens[2])};
}

OrderBookEntry CSVReader::stringsToOBE(std::string priceString,
                                       std::string amountString,
                                       std::string timestamp,
                                       std::string product,
                                       OrderBookType orderType)
{
    double price, amount;
    try
    {
        price = std::stod(priceString);
        amount = std::stod(amountString);
    }
    catch (const std::exception &e)
    {
        std::cout << "CSVReader::stringsToOBE Bad float! " << priceString << std::endl;
        std::cout << "CSVReader::stringsToOBE Bad float! " << amountString << std::endl;
        throw;
    }
    OrderBookEntry obe{price,
                       amount,
                       timestamp,
                       product,
                       orderType};

    return obe;
}
