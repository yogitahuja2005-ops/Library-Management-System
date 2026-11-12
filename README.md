# Library Management System (C++17 Console Application)

A single-file, menu-driven Library Management System designed for a college demonstration. It has no GUI, Qt dependency, CMake file, or third-party library.

## Build and run

```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o LibraryManagementSystem
./LibraryManagementSystem
```

The program creates `library_data.json` beside the executable on its first saved change. It automatically loads it on the next launch. Do not edit the file while the program is running.

## Features

- Add, list, and search books by title, author, or category.
- Register and list members.
- Issue books only when a verified copy is available; a member can have at most three active loans.
- Automatic 14-day due date and complete borrowing history.
- Returns enter **pending physical verification**. A found copy increases availability; a missing copy does not.
- Automatic overdue report with book, member, due date, and days late.
- Demand analysis ranks titles by historical borrow count and recommends copies for popular titles with low available stock.
- Dashboard and statistics report current library status.
- Validated menu and data input to prevent unsafe operations and incorrect stock counts.

## Data model in simple words

- `Book` stores total copies, verified available copies, and how often it has been borrowed.
- `Loan` records one issue transaction and its status.
- `Verification` is created on return. It provides the physical check trail (`PENDING`, `FOUND`, or `MISSING`).
- All four record groups are saved in the standard JSON file: books, members, loans, and verification records.

## Suggested demo

See [dummy_data.txt](dummy_data.txt). It gives a small presentation scenario and the exact menu actions. The application deliberately starts empty so your demonstration uses the IDs that it shows on screen.

## GitHub upload

Upload `main.cpp`, `README.md`, `dummy_data.txt`, and `.gitignore`. Build outputs and personal runtime data are excluded by `.gitignore`.
