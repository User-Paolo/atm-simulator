#pragma once
#include <string>

enum class TransactionType { SignUp, Login, Deposit, Withdraw, AdminRefill };

struct Transaction {
    std::string accountName;
    TransactionType type;
    long long amount = 0;
    std::string timestamp;
};
