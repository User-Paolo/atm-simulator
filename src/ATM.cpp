#include "ATM.h"
#include <algorithm>
#include <ctime>
#include <limits>

const std::string ADMIN_NAME = "Admin";
const std::string ADMIN_PASSWORD = "Secret";

ATM::ATM() {
    for (int d : DENOMINATIONS) {
        billSlots[d] = STARTING_COUNT_PER_SLOT;
    }
}

std::string ATM::currentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

void ATM::logTransaction(const std::string& name, TransactionType type, long long amount) {
    transactions.push_back(Transaction{name, type, amount, currentTimestamp()});
}

bool ATM::isValidAmount(long long amount) {
    return amount > 0 && amount <= MAX_TRANSACTION && amount % 5 == 0;
}

long long ATM::sumBills(const BillMap& bills) {
    long long total = 0;
    for (const auto& [denom, count] : bills) {
        total += static_cast<long long>(denom) * count;
    }
    return total;
}

bool ATM::accountExists(const std::string& name) const {
    return accounts.find(name) != accounts.end();
}

bool ATM::signUp(const std::string& name, const std::string& password, std::string& errorMsg) {
    if (name.empty() || password.empty()) {
        errorMsg = "Name and password cannot be empty.";
        return false;
    }
    if (name == ADMIN_NAME) {
        errorMsg = "'" + ADMIN_NAME + "' is a reserved name and cannot be used for a customer account.";
        return false;
    }
    if (accountExists(name)) {
        errorMsg = "An account with that name already exists. Try logging in instead.";
        return false;
    }
    accounts[name] = Account{name, password, 0};
    logTransaction(name, TransactionType::SignUp, 0);
    return true;
}

bool ATM::isAdmin(const std::string& name, const std::string& password) const {
    return name == ADMIN_NAME && password == ADMIN_PASSWORD;
}

bool ATM::login(const std::string& name, const std::string& password, std::string& errorMsg) {
    auto it = accounts.find(name);
    if (it == accounts.end()) {
        errorMsg = "No account found with that name.";
        return false;
    }
    if (it->second.password != password) {
        errorMsg = "Incorrect password.";
        return false;
    }
    logTransaction(name, TransactionType::Login, 0);
    return true;
}

std::optional<long long> ATM::getBalance(const std::string& name) const {
    auto it = accounts.find(name);
    if (it == accounts.end()) return std::nullopt;
    return it->second.balance;
}

// Fewest notes for amount, counted in 5 EUR units
bool ATM::findCombo(long long amount, const std::vector<int>& allowed,
                    bool respectAvailability, BillMap& outBills) const {
    const int units = static_cast<int>(amount / 5);
    const int NONE = std::numeric_limits<int>::max();
    std::vector<int> best(units + 1, NONE);
    std::vector<std::vector<int>> used(allowed.size(), std::vector<int>(units + 1, 0));
    best[0] = 0;

    for (size_t i = 0; i < allowed.size(); ++i) {
        const int step = allowed[i] / 5;
        const int limit = respectAvailability ? billSlots.at(allowed[i]) : units / step;
        std::vector<int> next = best;
        for (int u = step; u <= units; ++u) {
            for (int k = 1; k <= limit && k * step <= u; ++k) {
                const int prev = best[u - k * step];
                if (prev != NONE && prev + k < next[u]) {
                    next[u] = prev + k;
                    used[i][u] = k;
                }
            }
        }
        best = std::move(next);
    }
    if (best[units] == NONE) return false;

    outBills.clear();
    int u = units;
    for (int i = static_cast<int>(allowed.size()) - 1; i >= 0; --i) {
        const int k = used[i][u];
        if (k > 0) {
            outBills[allowed[i]] = k;
            u -= k * (allowed[i] / 5);
        }
    }
    return true;
}

std::vector<BillCombo> ATM::suggestCombos(long long amount, int maxWays, bool respectAvailability) const {
    std::vector<BillCombo> results;
    if (!isValidAmount(amount)) return results;

    struct Strategy { std::string label; std::vector<int> allowed; };
    std::vector<Strategy> strategies = {
        {"Fewest Bills",                  {100, 50, 20, 10, 5}},
        {"Small Notes Only (5s and 10s)", {10, 5}},
        {"Avoid EUR100 Notes",            {50, 20, 10, 5}},
    };

    for (const auto& s : strategies) {
        if (static_cast<int>(results.size()) >= maxWays) break;
        BillMap combo;
        if (!findCombo(amount, s.allowed, respectAvailability, combo)) continue;
        bool duplicate = std::any_of(results.begin(), results.end(),
                                     [&](const BillCombo& c) { return c.bills == combo; });
        if (!duplicate) results.push_back(BillCombo{s.label, combo});
    }
    return results;
}

std::vector<BillCombo> ATM::suggestWithdrawCombos(long long amount, int maxWays) const {
    return suggestCombos(amount, maxWays, true);
}

std::vector<BillCombo> ATM::suggestDepositCombos(long long amount, int maxWays) const {
    return suggestCombos(amount, maxWays, false);
}

bool ATM::validateBills(const BillMap& bills, std::string& errorMsg) const {
    for (const auto& [denom, count] : bills) {
        if (std::find(DENOMINATIONS.begin(), DENOMINATIONS.end(), denom) == DENOMINATIONS.end()) {
            errorMsg = std::to_string(denom) + " EUR is not a valid note.";
            return false;
        }
        if (count < 0) {
            errorMsg = "Note counts cannot be negative.";
            return false;
        }
    }
    return true;
}

bool ATM::withdraw(const std::string& name, const BillMap& bills, std::string& errorMsg) {
    auto accIt = accounts.find(name);
    if (accIt == accounts.end()) {
        errorMsg = "Account not found.";
        return false;
    }
    if (!validateBills(bills, errorMsg)) return false;

    long long amount = sumBills(bills);
    if (!isValidAmount(amount)) {
        errorMsg = "Withdrawal amount must be a multiple of 5, up to " + std::to_string(MAX_TRANSACTION) + " EUR.";
        return false;
    }
    if (amount > accIt->second.balance) {
        errorMsg = "Insufficient funds. Your balance is " + std::to_string(accIt->second.balance) + " EUR.";
        return false;
    }

    for (const auto& [denom, count] : bills) {
        int available = billSlots.at(denom);
        if (count > available) {
            errorMsg = "Not enough " + std::to_string(denom) + " EUR notes in the machine "
                       "(requested " + std::to_string(count) + ", only " + std::to_string(available) +
                       " available). Try a different combination of notes.";
            return false;
        }
    }

    for (const auto& [denom, count] : bills) {
        billSlots[denom] -= count;
    }
    accIt->second.balance -= amount;
    logTransaction(name, TransactionType::Withdraw, amount);
    return true;
}

bool ATM::deposit(const std::string& name, const BillMap& bills, std::string& errorMsg) {
    auto accIt = accounts.find(name);
    if (accIt == accounts.end()) {
        errorMsg = "Account not found.";
        return false;
    }
    if (!validateBills(bills, errorMsg)) return false;

    long long amount = sumBills(bills);
    if (!isValidAmount(amount)) {
        errorMsg = "Deposit amount must be a multiple of 5, up to " + std::to_string(MAX_TRANSACTION) + " EUR.";
        return false;
    }

    for (const auto& [denom, count] : bills) {
        billSlots[denom] += count;
    }
    accIt->second.balance += amount;
    logTransaction(name, TransactionType::Deposit, amount);
    return true;
}

long long ATM::getTotalATMBalance() const {
    return sumBills(billSlots);
}

const BillMap& ATM::getBillSlots() const {
    return billSlots;
}

std::vector<Transaction> ATM::getRecentTransactions(int count) const {
    size_t n = std::min(static_cast<size_t>(std::max(count, 0)), transactions.size());
    return std::vector<Transaction>(transactions.rbegin(), transactions.rbegin() + n);
}

bool ATM::adminRefill(const BillMap& bills, std::string& errorMsg) {
    if (!validateBills(bills, errorMsg)) return false;

    long long amount = sumBills(bills);
    if (amount == 0) {
        errorMsg = "No notes entered, nothing to refill.";
        return false;
    }
    for (const auto& [denom, count] : bills) {
        billSlots[denom] += count;
    }
    logTransaction(ADMIN_NAME, TransactionType::AdminRefill, amount);
    return true;
}
