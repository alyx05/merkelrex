#pragma once
#include <string>

// minimal user profile used for authentication and persistence
struct User {
    std::string username;
    std::string fullName;
    std::string email;
    size_t passwordHash;
};