#include <iostream>
#include <limits>
#include "ATM.h"
#include "UI.h"

static void waitForEnter() {
    std::cout << "\n" << UI::YELLOW << "Press Enter to continue..." << UI::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string typeName(TransactionType type) {
    switch (type) {
        case TransactionType::SignUp:      return "SIGNUP";
        case TransactionType::Login:       return "LOGIN";
        case TransactionType::Deposit:     return "DEPOSIT";
        case TransactionType::Withdraw:    return "WITHDRAW";
        case TransactionType::AdminRefill: return "ADMIN_REFILL";
    }
    return "UNKNOWN";
}

static void printComboList(const std::vector<BillCombo>& combos) {
    for (size_t i = 0; i < combos.size(); ++i) {
        std::cout << "  " << UI::BOLD << (i + 1) << ") " << combos[i].label << UI::RESET << "\n     -> ";
        bool first = true;
        for (auto it = combos[i].bills.rbegin(); it != combos[i].bills.rend(); ++it) {
            if (!first) std::cout << " + ";
            std::cout << it->second << " x " << it->first << "EUR";
            first = false;
        }
        std::cout << "\n";
    }
}

// Pick a suggested combo or enter a custom one
static bool chooseBillBreakdown(const std::vector<BillCombo>& combos, long long amount, BillMap& outBills) {
    std::cout << "\nSuggested ways to make " << UI::euros(amount) << ":\n";
    printComboList(combos);
    std::cout << "  " << UI::BOLD << "0) Enter a custom bill breakdown" << UI::RESET << "\n";

    int choice = -1;
    while (true) {
        if (!UI::readInt("Choose an option: ", choice)) {
            UI::error("Please enter a number.");
            continue;
        }
        if (choice == 0) break;
        if (choice >= 1 && choice <= static_cast<int>(combos.size())) {
            outBills = combos[choice - 1].bills;
            return true;
        }
        UI::error("Invalid choice.");
    }

    outBills.clear();
    for (int denom : ATM::DENOMINATIONS) {
        int count = 0;
        std::string prompt = "  How many " + std::to_string(denom) + "EUR notes? ";
        while (!UI::readInt(prompt, count) || count < 0) {
            UI::error("Enter a non-negative whole number.");
        }
        if (count > 0) outBills[denom] = count;
    }
    long long total = ATM::sumBills(outBills);
    if (total != amount) {
        UI::error("Those notes add up to " + UI::euros(total) + ", not " + UI::euros(amount) + ".");
        return false;
    }
    return true;
}

static bool readValidAmount(const std::string& prompt, long long& amount) {
    if (!UI::readAmount(prompt, amount) || !ATM::isValidAmount(amount)) {
        UI::error("Amount must be a multiple of 5, up to " + UI::euros(ATM::MAX_TRANSACTION) + ".");
        return false;
    }
    return true;
}

static void doViewBalance(ATM& atm, const std::string& name) {
    UI::printBox({"Account: " + name, "Balance: " + UI::euros(atm.getBalance(name).value())});
}

static void doDeposit(ATM& atm, const std::string& name) {
    long long amount = 0;
    if (!readValidAmount("Amount to deposit: ", amount)) return;

    BillMap bills;
    if (!chooseBillBreakdown(atm.suggestDepositCombos(amount), amount, bills)) return;

    std::string err;
    if (atm.deposit(name, bills, err)) {
        UI::success("Deposited " + UI::euros(amount) + ". New balance: " + UI::euros(atm.getBalance(name).value()));
    } else {
        UI::error(err);
    }
}

static void doWithdraw(ATM& atm, const std::string& name) {
    long long amount = 0;
    if (!readValidAmount("Amount to withdraw: ", amount)) return;

    long long balance = atm.getBalance(name).value();
    if (amount > balance) {
        UI::error("Insufficient funds. Your balance is " + UI::euros(balance) + ".");
        return;
    }
    auto combos = atm.suggestWithdrawCombos(amount);
    if (combos.empty()) {
        UI::error("The machine does not currently have the notes to dispense " + UI::euros(amount) +
                  ". Try a different amount.");
        return;
    }
    BillMap bills;
    if (!chooseBillBreakdown(combos, amount, bills)) return;

    std::string err;
    if (atm.withdraw(name, bills, err)) {
        UI::success("Withdrew " + UI::euros(amount) + ". New balance: " + UI::euros(atm.getBalance(name).value()));
    } else {
        UI::error(err);
    }
}

static void customerMenu(ATM& atm, const std::string& name) {
    while (true) {
        UI::printBanner("Welcome, " + name);
        UI::printBox({
            "1) View Balance",
            "2) Deposit",
            "3) Withdraw",
            "4) Log Out"
        });
        int choice = 0;
        if (!UI::readInt("Select an option: ", choice)) { UI::error("Enter a number."); continue; }
        switch (choice) {
            case 1: doViewBalance(atm, name); waitForEnter(); break;
            case 2: doDeposit(atm, name); waitForEnter(); break;
            case 3: doWithdraw(atm, name); waitForEnter(); break;
            case 4: return;
            default: UI::error("Invalid option."); break;
        }
    }
}

static void adminViewTotal(ATM& atm) {
    UI::printBox({"Total cash held by machine:", UI::euros(atm.getTotalATMBalance())});
}

static void adminViewSlots(ATM& atm) {
    const BillMap& slots = atm.getBillSlots();
    std::vector<std::string> lines = {"Bill slot contents:"};
    for (auto it = slots.rbegin(); it != slots.rend(); ++it) {
        lines.push_back("  " + std::to_string(it->first) + " EUR notes: " + std::to_string(it->second));
    }
    UI::printBox(lines);
}

static void adminViewHistory(ATM& atm) {
    auto history = atm.getRecentTransactions(10);
    if (history.empty()) {
        UI::info("No transactions recorded yet.");
        return;
    }
    std::vector<std::string> lines = {"Last " + std::to_string(history.size()) + " transaction(s), newest first:"};
    for (const auto& t : history) {
        std::string line = t.timestamp + "  " + typeName(t.type);
        if (t.amount > 0) line += "  " + UI::euros(t.amount);
        lines.push_back(line + "  " + t.accountName);
    }
    UI::printBox(lines);
}

static void adminRefill(ATM& atm) {
    UI::info("Refilling the machine's own note supply (not tied to a customer account).");
    BillMap bills;
    for (int denom : ATM::DENOMINATIONS) {
        int count = 0;
        std::string prompt = "  How many " + std::to_string(denom) + "EUR notes to add? ";
        while (!UI::readInt(prompt, count) || count < 0) {
            UI::error("Enter a non-negative whole number.");
        }
        if (count > 0) bills[denom] = count;
    }
    std::string err;
    if (atm.adminRefill(bills, err)) {
        UI::success("Machine refilled. New total: " + UI::euros(atm.getTotalATMBalance()));
    } else {
        UI::error(err);
    }
}

static void adminMenu(ATM& atm) {
    while (true) {
        UI::printBanner("Admin Panel");
        UI::printBox({
            "1) View Total Machine Balance",
            "2) View Bill Counts per Slot",
            "3) View Last 10 Transactions",
            "4) Deposit / Refill Machine",
            "5) Log Out"
        });
        int choice = 0;
        if (!UI::readInt("Select an option: ", choice)) { UI::error("Enter a number."); continue; }
        switch (choice) {
            case 1: adminViewTotal(atm); waitForEnter(); break;
            case 2: adminViewSlots(atm); waitForEnter(); break;
            case 3: adminViewHistory(atm); waitForEnter(); break;
            case 4: adminRefill(atm); waitForEnter(); break;
            case 5: return;
            default: UI::error("Invalid option."); break;
        }
    }
}

static void doSignUp(ATM& atm) {
    std::string name = UI::readLine("Choose a username: ");
    std::string pass = UI::readLine("Choose a password: ");
    std::string err;
    if (atm.signUp(name, pass, err)) {
        UI::success("Account created for " + name + ". You can now log in.");
    } else {
        UI::error(err);
    }
    waitForEnter();
}

static void doLogIn(ATM& atm) {
    std::string name = UI::readLine("Username: ");
    std::string pass = UI::readLine("Password: ");

    if (atm.isAdmin(name, pass)) {
        UI::success("Admin access granted.");
        adminMenu(atm);
        return;
    }

    std::string err;
    if (atm.login(name, pass, err)) {
        customerMenu(atm, name);
    } else {
        UI::error(err);
        waitForEnter();
    }
}

int main() {
    UI::enableColors();
    ATM atm;

    while (true) {
        UI::printBanner("EURO ATM SIMULATOR");
        UI::printBox({
            "1) Sign Up",
            "2) Log In",
            "3) Exit"
        });
        int choice = 0;
        if (!UI::readInt("Select an option: ", choice)) { UI::error("Enter a number."); continue; }
        switch (choice) {
            case 1: doSignUp(atm); break;
            case 2: doLogIn(atm); break;
            case 3:
                std::cout << UI::CYAN << "\nThank you for using the ATM. Goodbye!\n" << UI::RESET;
                return 0;
            default: UI::error("Invalid option."); break;
        }
    }
}
