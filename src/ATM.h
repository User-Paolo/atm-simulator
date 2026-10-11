#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "Account.h"
#include "Transaction.h"

using BillMap = std::map<int, int>; // denomination -> note count

struct BillCombo {
    std::string label;
    BillMap bills;
};

class ATM {
public:
    ATM();

    static constexpr std::array<int, 5> DENOMINATIONS = {100, 50, 20, 10, 5};
    static constexpr int STARTING_COUNT_PER_SLOT = 100;
    static constexpr long long MAX_TRANSACTION = 10000;

    bool signUp(const std::string& name, const std::string& password, std::string& errorMsg);
    bool login(const std::string& name, const std::string& password, std::string& errorMsg);
    bool isAdmin(const std::string& name, const std::string& password) const;
    bool accountExists(const std::string& name) const;
    std::optional<long long> getBalance(const std::string& name) const;

    std::vector<BillCombo> suggestWithdrawCombos(long long amount, int maxWays = 3) const;
    std::vector<BillCombo> suggestDepositCombos(long long amount, int maxWays = 3) const;

    bool withdraw(const std::string& name, const BillMap& bills, std::string& errorMsg);
    bool deposit(const std::string& name, const BillMap& bills, std::string& errorMsg);

    long long getTotalATMBalance() const;
    const BillMap& getBillSlots() const;
    std::vector<Transaction> getRecentTransactions(int count = 10) const;
    bool adminRefill(const BillMap& bills, std::string& errorMsg);

    static long long sumBills(const BillMap& bills);
    static bool isValidAmount(long long amount);

private:
    std::map<std::string, Account> accounts;
    BillMap billSlots;
    std::vector<Transaction> transactions;

    void logTransaction(const std::string& name, TransactionType type, long long amount);
    static std::string currentTimestamp();
    bool validateBills(const BillMap& bills, std::string& errorMsg) const;
    std::vector<BillCombo> suggestCombos(long long amount, int maxWays, bool respectAvailability) const;
    bool findCombo(long long amount, const std::vector<int>& allowed,
                   bool respectAvailability, BillMap& outBills) const;
};
