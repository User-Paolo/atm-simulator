#pragma once
#include <string>

// Represents a single customer account.
struct Account {
    std::string name;
    std::string password;
    long long balance = 0;
};
