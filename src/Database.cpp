#include "Database.h"
#include <stdexcept>
#include <iostream>

namespace
{
    // RAII wrapper so a thrown exception mid-query can't leak a prepared statement
    struct Stmt
    {
        sqlite3_stmt* p = nullptr;
        Stmt(sqlite3* db, const std::string& sql)
        {
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &p, nullptr) != SQLITE_OK)
                throw std::runtime_error("sqlite3_prepare_v2 failed: " + std::string(sqlite3_errmsg(db)));
        }
        ~Stmt() { sqlite3_finalize(p); }
        operator sqlite3_stmt*() const { return p; }
    };
}

Database::Database(const std::string& dbPath)
{
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
    {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error("Failed to open database: " + msg);
    }
    exec("PRAGMA foreign_keys = ON;");
    exec("PRAGMA journal_mode = WAL;"); // better concurrent read/write behaviour once the API layer lands
    initSchema();
}

Database::~Database()
{
    if (db) sqlite3_close(db);
}

void Database::exec(const std::string& sql)
{
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL error: " + msg);
    }
}

void Database::initSchema()
{
    exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            username        TEXT PRIMARY KEY,
            full_name       TEXT NOT NULL,
            email           TEXT NOT NULL,
            password_hash   TEXT NOT NULL
        );
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS wallet_balances (
            username    TEXT NOT NULL,
            currency    TEXT NOT NULL,
            amount      REAL NOT NULL,
            PRIMARY KEY (username, currency),
            FOREIGN KEY (username) REFERENCES users(username)
        );
    )");

    exec(R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            username    TEXT NOT NULL,
            timestamp   TEXT NOT NULL,
            product     TEXT NOT NULL,
            type        TEXT NOT NULL,
            price       REAL NOT NULL,
            amount      REAL NOT NULL,
            total       REAL NOT NULL,
            FOREIGN KEY (username) REFERENCES users(username)
        );
    )");

    exec("CREATE INDEX IF NOT EXISTS idx_transactions_username ON transactions(username);");
}

// ---------------- users ----------------

bool Database::userExists(const std::string& fullName, const std::string& email)
{
    Stmt stmt(db, "SELECT 1 FROM users WHERE full_name = ? AND email = ? LIMIT 1;");
    sqlite3_bind_text(stmt, 1, fullName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(stmt) == SQLITE_ROW;
}

bool Database::loadUser(const std::string& username, User& outUser)
{
    Stmt stmt(db, "SELECT username, full_name, email, password_hash FROM users WHERE username = ?;");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) return false;

    outUser.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    outUser.fullName  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    outUser.email     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    std::string hashStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    outUser.passwordHash = std::stoull(hashStr);
    return true;
}

void Database::saveUser(const User& user)
{
    Stmt stmt(db, "INSERT INTO users(username, full_name, email, password_hash) VALUES(?, ?, ?, ?);");
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.fullName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, std::to_string(user.passwordHash).c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error("Failed to insert user: " + std::string(sqlite3_errmsg(db)));
}

void Database::updatePassword(const std::string& username, size_t newPasswordHash)
{
    Stmt stmt(db, "UPDATE users SET password_hash = ? WHERE username = ?;");
    sqlite3_bind_text(stmt, 1, std::to_string(newPasswordHash).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error("Failed to update password: " + std::string(sqlite3_errmsg(db)));
}

// ---------------- wallet ----------------

void Database::saveInitialWallet(const std::string& username, double bonusAmount)
{
    Stmt stmt(db, R"(
        INSERT INTO wallet_balances(username, currency, amount) VALUES(?, 'USDT', ?)
        ON CONFLICT(username, currency) DO UPDATE SET amount = excluded.amount;
    )");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, bonusAmount);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error("Failed to seed wallet: " + std::string(sqlite3_errmsg(db)));
}

std::map<std::string, double> Database::loadWalletBalances(const std::string& username)
{
    std::map<std::string, double> balances;
    Stmt stmt(db, "SELECT currency, amount FROM wallet_balances WHERE username = ?;");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double amount = sqlite3_column_double(stmt, 1);
        balances[currency] = amount;
    }
    return balances;
}

void Database::syncWalletBalances(const std::string& username,
                                   const std::map<std::string, double>& balances)
{
    // Wrap the delete+reinsert in a transaction so a crash mid-sync can't
    // leave a user with an empty wallet -- this is the actual ACID payoff
    // the phase brief was asking for.
    exec("BEGIN TRANSACTION;");
    try
    {
        {
            Stmt del(db, "DELETE FROM wallet_balances WHERE username = ?;");
            sqlite3_bind_text(del, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(del);
        }
        for (const auto& [currency, amount] : balances)
        {
            Stmt ins(db, "INSERT INTO wallet_balances(username, currency, amount) VALUES(?, ?, ?);");
            sqlite3_bind_text(ins, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(ins, 2, currency.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(ins, 3, amount);
            sqlite3_step(ins);
        }
        exec("COMMIT;");
    }
    catch (...)
    {
        exec("ROLLBACK;");
        throw;
    }
}

// ---------------- transactions ----------------

void Database::logTransaction(const TransactionRecord& tx)
{
    Stmt stmt(db, R"(
        INSERT INTO transactions(username, timestamp, product, type, price, amount, total)
        VALUES(?, ?, ?, ?, ?, ?, ?);
    )");
    sqlite3_bind_text(stmt, 1, tx.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, tx.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, tx.product.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, tx.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, tx.price);
    sqlite3_bind_double(stmt, 6, tx.amount);
    sqlite3_bind_double(stmt, 7, tx.total);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error("Failed to log transaction: " + std::string(sqlite3_errmsg(db)));
}

std::vector<TransactionRecord> Database::getRecentTransactions(const std::string& username, int limit)
{
    std::vector<TransactionRecord> results;
    Stmt stmt(db, R"(
        SELECT username, timestamp, product, type, price, amount, total
        FROM transactions WHERE username = ?
        ORDER BY id DESC LIMIT ?;
    )");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        TransactionRecord tx;
        tx.username  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tx.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tx.product   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        tx.type      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        tx.price     = sqlite3_column_double(stmt, 4);
        tx.amount    = sqlite3_column_double(stmt, 5);
        tx.total     = sqlite3_column_double(stmt, 6);
        results.push_back(tx);
    }
    return results;
}

std::vector<TransactionRecord> Database::getTransactionsForUser(const std::string& username,
                                                                  const std::string& productFilter,
                                                                  const std::string& startDate,
                                                                  const std::string& endDate)
{
    std::vector<TransactionRecord> results;
    Stmt stmt(db, R"(
        SELECT username, timestamp, product, type, price, amount, total
        FROM transactions
        WHERE username = ?
          AND (? = '' OR product = ?)
          AND (? = '' OR substr(timestamp, 1, 10) >= ?)
          AND (? = '' OR substr(timestamp, 1, 10) <= ?)
        ORDER BY id ASC;
    )");
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, productFilter.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, productFilter.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, endDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, endDate.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        TransactionRecord tx;
        tx.username  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tx.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tx.product   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        tx.type      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        tx.price     = sqlite3_column_double(stmt, 4);
        tx.amount    = sqlite3_column_double(stmt, 5);
        tx.total     = sqlite3_column_double(stmt, 6);
        results.push_back(tx);
    }
    return results;
}

std::vector<OrderBookEntry> Database::getAllSaleEntries()
{
    std::vector<OrderBookEntry> entries;
    Stmt stmt(db, R"(
        SELECT username, timestamp, product, type, price, amount
        FROM transactions
        WHERE type = 'asksale' OR type = 'bidsale';
    )");

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string username  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string product   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string typeStr   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        double price  = sqlite3_column_double(stmt, 4);
        double amount = sqlite3_column_double(stmt, 5);

        OrderBookType type = (typeStr == "asksale") ? OrderBookType::asksale : OrderBookType::bidsale;
        entries.push_back(OrderBookEntry{price, amount, timestamp, product, type, username});
    }
    return entries;
}

void Database::settleSale(const OrderBookEntry& sale)
{
    if (sale.username.empty() || sale.username == "dataset")
    {
        return; // not a real, attributable order -- nothing to settle
    }

    User owner;
    if (!loadUser(sale.username, owner))
    {
        return; // orphan username somehow made it into a live order -- skip rather than crash
    }

    Wallet w;
    for (const auto& [currency, bal] : loadWalletBalances(sale.username))
    {
        w.insertCurrency(currency, bal);
    }

    OrderBookEntry saleCopy = sale; // Wallet::processSale takes a non-const reference
    w.processSale(saleCopy);
    syncWalletBalances(sale.username, w.getBalances());

    TransactionRecord tx;
    tx.username = sale.username;
    tx.timestamp = sale.timestamp;
    tx.product = sale.product;
    tx.type = (sale.orderType == OrderBookType::asksale) ? "asksale" : "bidsale";
    tx.price = sale.price;
    tx.amount = sale.amount;
    tx.total = sale.price * sale.amount;
    logTransaction(tx);
}