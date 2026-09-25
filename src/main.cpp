#include "Wallet.h"
#include <iostream>
#include <string>
#include "MerkelMain.h"
#include "Database.h"
#include "OrderBook.h"
#include "ApiServer.h"

int main(int argc, char* argv[])
{
    bool runCli = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--cli") runCli = true;
    }

    if (runCli)
    {
        MerkelMain app{};
        app.init();
    }
    else
    {
        Database db("data/merkelrex.db");
        OrderBook orderBook("data/20200601.csv", db);
        ApiServer api(db, orderBook);
        api.run(18080);
    }
}