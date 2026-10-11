# Euro ATM Simulator (C++)

Originally built in 2022 for university coursework.

A command-line ATM simulator supporting customer sign-up/login, deposits,
withdrawals, and an admin panel for machine management.

## Features

- **Customer accounts**: sign up and log in with a name + password.
- **Balance**: always non-negative, viewable on demand.
- **Deposits & withdrawals**: driven by the machine's actual note inventory.
- **Bill slots**: EUR 5 / 10 / 20 / 50 / 100 notes, each starting with 100
  bills, no upper capacity.
- **Smart suggestions**: for both deposits and withdrawals, the ATM proposes
  up to 3 popular ways to make up the amount (fewest bills, small notes
  only, or avoiding EUR100 notes) — or you can enter a custom breakdown.
- **Admin mode** (login `Admin` / `Secret`):
  - View the machine's total cash balance
  - View how many notes are in each slot
  - View the last 10 transactions (who, what, how much)
  - Refill the machine's note supply
- **Error handling** throughout: duplicate signups, wrong passwords,
  invalid/non-multiple-of-5 amounts, insufficient account funds, and
  insufficient notes in a specific slot (with a nudge toward other options).
- A boxed, colored CLI "UI" using Unicode box-drawing characters and ANSI
  colors.

## Project structure

```
src/
  Account.h       Customer account data
  Transaction.h   Logged event data
  ATM.h / ATM.cpp Core banking + bill-dispensing logic
  UI.h            CLI presentation helpers (boxes, banners, colored output)
  main.cpp        Menu loop tying it all together
```

## Building

Requires a C++17 compiler (g++ or clang++).

```
g++ -std=c++17 -O2 -o atm_simulator src/main.cpp src/ATM.cpp
./atm_simulator
```

## Notes / known simplifications

- Data is in-memory only — nothing persists once the program exits. Adding
  file- or SQLite-based persistence would be a natural next step.
- Passwords are stored in plaintext for simplicity; a real system would
  hash them (e.g. with bcrypt) before storing.
- Amounts must be whole-euro multiples of 5, since that's the smallest note
  the machine holds.
