#include <gtest/gtest.h>
#include "ATM.h"

class AtmTest : public ::testing::Test {
protected:
    ATM atm;
    std::string err;

    void SetUp() override {
        ASSERT_TRUE(atm.signUp("bob", "pw", err));
    }
};

TEST(AmountTest, AcceptsPositiveMultiplesOfFiveUpToLimit) {
    EXPECT_TRUE(ATM::isValidAmount(5));
    EXPECT_TRUE(ATM::isValidAmount(ATM::MAX_TRANSACTION));
    EXPECT_FALSE(ATM::isValidAmount(0));
    EXPECT_FALSE(ATM::isValidAmount(-5));
    EXPECT_FALSE(ATM::isValidAmount(12));
    EXPECT_FALSE(ATM::isValidAmount(ATM::MAX_TRANSACTION + 5));
}

TEST_F(AtmTest, SignUpStartsWithZeroBalance) {
    EXPECT_EQ(atm.getBalance("bob"), 0);
}

TEST_F(AtmTest, SignUpRejectsDuplicateEmptyAndReservedNames) {
    EXPECT_FALSE(atm.signUp("bob", "other", err));
    EXPECT_FALSE(atm.signUp("", "pw", err));
    EXPECT_FALSE(atm.signUp("alice", "", err));
    EXPECT_FALSE(atm.signUp("Admin", "pw", err));
}

TEST_F(AtmTest, LoginChecksPassword) {
    EXPECT_TRUE(atm.login("bob", "pw", err));
    EXPECT_FALSE(atm.login("bob", "wrong", err));
    EXPECT_FALSE(atm.login("ghost", "pw", err));
}

TEST_F(AtmTest, AdminNeedsExactCredentials) {
    EXPECT_TRUE(atm.isAdmin("Admin", "Secret"));
    EXPECT_FALSE(atm.isAdmin("admin", "Secret"));
    EXPECT_FALSE(atm.isAdmin("Admin", "secret"));
}

TEST_F(AtmTest, UnknownAccountHasNoBalance) {
    EXPECT_FALSE(atm.getBalance("ghost").has_value());
}

TEST_F(AtmTest, MachineStartsWithFullSlots) {
    for (int denom : ATM::DENOMINATIONS) {
        EXPECT_EQ(atm.getBillSlots().at(denom), ATM::STARTING_COUNT_PER_SLOT);
    }
    EXPECT_EQ(atm.getTotalATMBalance(), 18500);
}

TEST_F(AtmTest, DepositUpdatesBalanceAndSlots) {
    ASSERT_TRUE(atm.deposit("bob", {{50, 2}, {20, 1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 120);
    EXPECT_EQ(atm.getBillSlots().at(50), 102);
    EXPECT_EQ(atm.getBillSlots().at(20), 101);
}

TEST_F(AtmTest, DepositRejectsNegativeNoteCounts) {
    EXPECT_FALSE(atm.deposit("bob", {{100, 2}, {50, -1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 0);
    EXPECT_EQ(atm.getBillSlots().at(100), 100);
}

TEST_F(AtmTest, DepositRejectsUnknownNotes) {
    EXPECT_FALSE(atm.deposit("bob", {{7, 5}}, err));
    EXPECT_EQ(atm.getBillSlots().count(7), 0u);
}

TEST_F(AtmTest, DepositRejectsAmountOverLimit) {
    EXPECT_FALSE(atm.deposit("bob", {{100, 101}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 0);
}

TEST_F(AtmTest, WithdrawUpdatesBalanceAndSlots) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 5}}, err));
    ASSERT_TRUE(atm.withdraw("bob", {{100, 1}, {50, 1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 350);
    EXPECT_EQ(atm.getBillSlots().at(50), 99);
}

TEST_F(AtmTest, WithdrawRejectsInsufficientFunds) {
    ASSERT_TRUE(atm.deposit("bob", {{50, 1}}, err));
    EXPECT_FALSE(atm.withdraw("bob", {{100, 1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 50);
}

TEST_F(AtmTest, WithdrawRejectsMoreNotesThanMachineHolds) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 50}}, err));
    ASSERT_TRUE(atm.withdraw("bob", {{20, 100}}, err));
    EXPECT_FALSE(atm.withdraw("bob", {{20, 1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 3000);
}

TEST_F(AtmTest, WithdrawRejectsNegativeNoteCounts) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 1}}, err));
    EXPECT_FALSE(atm.withdraw("bob", {{100, 1}, {5, -1}}, err));
    EXPECT_EQ(atm.getBalance("bob"), 100);
    EXPECT_EQ(atm.getBillSlots().at(5), 100);
}

TEST_F(AtmTest, FewestBillsWithFullSlots) {
    auto combos = atm.suggestWithdrawCombos(185);
    ASSERT_FALSE(combos.empty());
    EXPECT_EQ(combos[0].bills, (BillMap{{100, 1}, {50, 1}, {20, 1}, {10, 1}, {5, 1}}));
}

TEST_F(AtmTest, FindsCombinationWhenLargestNoteFirstFails) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 50}}, err));
    ASSERT_TRUE(atm.withdraw("bob", {{10, 100}, {5, 100}}, err));
    auto combos = atm.suggestWithdrawCombos(60);
    ASSERT_FALSE(combos.empty());
    EXPECT_EQ(combos[0].bills, (BillMap{{20, 3}}));
}

TEST_F(AtmTest, NoWithdrawSuggestionWhenMachineCannotPay) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 50}}, err));
    ASSERT_TRUE(atm.withdraw("bob", {{20, 100}, {10, 100}, {5, 100}}, err));
    EXPECT_TRUE(atm.suggestWithdrawCombos(60).empty());
}

TEST_F(AtmTest, DepositSuggestionsIgnoreMachineStock) {
    ASSERT_TRUE(atm.deposit("bob", {{100, 50}}, err));
    ASSERT_TRUE(atm.withdraw("bob", {{10, 100}, {5, 100}}, err));
    auto combos = atm.suggestDepositCombos(15);
    ASSERT_FALSE(combos.empty());
    EXPECT_EQ(combos[0].bills, (BillMap{{10, 1}, {5, 1}}));
}

TEST_F(AtmTest, HugeAmountGetsNoSuggestions) {
    EXPECT_TRUE(atm.suggestDepositCombos(1000000000000000LL).empty());
    EXPECT_TRUE(atm.suggestWithdrawCombos(1000000000000000LL).empty());
}

TEST_F(AtmTest, EverySuggestionAddsUpAndFitsStock) {
    for (long long amount = 5; amount <= 1000; amount += 5) {
        auto combos = atm.suggestWithdrawCombos(amount);
        ASSERT_FALSE(combos.empty()) << amount;
        for (size_t i = 0; i < combos.size(); ++i) {
            EXPECT_EQ(ATM::sumBills(combos[i].bills), amount) << combos[i].label;
            for (const auto& [denom, count] : combos[i].bills) {
                EXPECT_LE(count, atm.getBillSlots().at(denom));
            }
            for (size_t j = 0; j < i; ++j) {
                EXPECT_NE(combos[i].bills, combos[j].bills);
            }
        }
    }
}

TEST_F(AtmTest, AdminRefillAddsNotesWithoutLimit) {
    ASSERT_TRUE(atm.adminRefill({{100, 500}}, err));
    EXPECT_EQ(atm.getBillSlots().at(100), 600);
}

TEST_F(AtmTest, AdminRefillRejectsEmptyAndInvalidInput) {
    EXPECT_FALSE(atm.adminRefill({}, err));
    EXPECT_FALSE(atm.adminRefill({{100, -1}}, err));
    EXPECT_FALSE(atm.adminRefill({{7, 1}}, err));
}

TEST_F(AtmTest, RecentTransactionsAreNewestFirst) {
    ASSERT_TRUE(atm.login("bob", "pw", err));
    ASSERT_TRUE(atm.deposit("bob", {{50, 1}}, err));
    auto history = atm.getRecentTransactions(2);
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].type, TransactionType::Deposit);
    EXPECT_EQ(history[0].amount, 50);
    EXPECT_EQ(history[1].type, TransactionType::Login);
    EXPECT_EQ(atm.getRecentTransactions(100).size(), 3u);
    EXPECT_TRUE(atm.getRecentTransactions(-1).empty());
}
