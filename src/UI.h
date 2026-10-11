#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace UI {

    // ANSI color codes
    inline const std::string RESET   = "\033[0m";
    inline const std::string BOLD    = "\033[1m";
    inline const std::string GREEN   = "\033[32m";
    inline const std::string RED     = "\033[31m";
    inline const std::string YELLOW  = "\033[33m";
    inline const std::string CYAN    = "\033[36m";
    inline const std::string MAGENTA = "\033[35m";

    inline constexpr int WIDTH = 60;

    inline void enableColors() {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(out, &mode)) {
            SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#endif
    }

    inline void printBanner(const std::string& title) {
        std::string trimmed = title.substr(0, WIDTH - 4);
        std::cout << CYAN;
        std::cout << "+" << std::string(WIDTH - 2, '=') << "+\n";
        int pad = static_cast<int>(WIDTH - 2 - trimmed.size());
        int left = pad / 2;
        int right = pad - left;
        std::cout << "|" << std::string(left, ' ') << BOLD << trimmed << RESET << CYAN
                   << std::string(right, ' ') << "|\n";
        std::cout << "+" << std::string(WIDTH - 2, '=') << "+\n" << RESET;
    }

    inline void printBox(const std::vector<std::string>& lines) {
        std::cout << "+" << std::string(WIDTH - 2, '-') << "+\n";
        for (const auto& line : lines) {
            std::string trimmed = line.substr(0, WIDTH - 4);
            int pad = static_cast<int>(WIDTH - 4 - trimmed.size());
            std::cout << "| " << trimmed << std::string(pad, ' ') << " |\n";
        }
        std::cout << "+" << std::string(WIDTH - 2, '-') << "+\n";
    }

    inline void success(const std::string& msg) {
        std::cout << GREEN << "  [OK] " << msg << RESET << "\n";
    }

    inline void error(const std::string& msg) {
        std::cout << RED << "  [!] " << msg << RESET << "\n";
    }

    inline void info(const std::string& msg) {
        std::cout << YELLOW << "  [i] " << msg << RESET << "\n";
    }

    inline std::string euros(long long amount) {
        return std::to_string(amount) + " EUR";
    }

    // Exits cleanly on EOF instead of looping forever
    inline std::string readLine(const std::string& prompt) {
        std::cout << MAGENTA << prompt << RESET;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n" << CYAN << "Input closed. Goodbye!\n" << RESET;
            std::exit(0);
        }
        return line;
    }

    inline bool readAmount(const std::string& prompt, long long& out) {
        std::string line = readLine(prompt);
        try {
            size_t pos;
            long long value = std::stoll(line, &pos);
            if (pos != line.size()) return false;
            out = value;
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool readInt(const std::string& prompt, int& out) {
        std::string line = readLine(prompt);
        try {
            size_t pos;
            int value = std::stoi(line, &pos);
            if (pos != line.size()) return false;
            out = value;
            return true;
        } catch (...) {
            return false;
        }
    }

} // namespace UI
