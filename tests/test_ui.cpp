#include <gtest/gtest.h>
#include <sstream>
#include "UI.h"

TEST(UiTest, LongBannerTitleStaysInsideBox) {
    std::ostringstream captured;
    std::streambuf* original = std::cout.rdbuf(captured.rdbuf());
    UI::printBanner("Welcome, " + std::string(80, 'x'));
    std::cout.rdbuf(original);

    std::istringstream lines(captured.str());
    std::string line;
    int boxLines = 0;
    while (std::getline(lines, line)) {
        std::string visible;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '\033') {
                while (i < line.size() && line[i] != 'm') ++i;
            } else {
                visible += line[i];
            }
        }
        if (visible.empty()) continue;
        EXPECT_EQ(visible.size(), static_cast<size_t>(UI::WIDTH));
        ++boxLines;
    }
    EXPECT_EQ(boxLines, 3);
}

TEST(UiTest, EurosFormatsWholeAmounts) {
    EXPECT_EQ(UI::euros(50), "50 EUR");
    EXPECT_EQ(UI::euros(0), "0 EUR");
}
