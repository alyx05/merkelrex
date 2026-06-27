#pragma once
#include <string>

struct User {
    std::string username;
    std::string fullName;
    std::string email;
    size_t passwordHash;
};