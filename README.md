# Library Management System

A menu-driven Library Management System developed in **C++17**. It provides book management, member registration, borrowing, returns, physical verification, overdue tracking, and library statistics.

## Features

- Add, list, and search books by title, author, or category.
- Register and list library members.
- Issue books only when a verified copy is available.
- Automatic 14-day due dates and complete borrowing history.
- Returns require pending physical verification before a copy becomes available again.
- Automatic overdue reports showing book, member, due date, and days overdue.
- Demand analysis ranks books by historical borrowing count.
- Dashboard and statistics showing current library status.
- Input validation to prevent invalid operations.

## Build and Run

Make sure you have GCC/G++ installed.

```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o LibraryManagementSystem
./LibraryManagementSystem
```

## Data Storage

The application automatically creates and updates `library_data.json` beside the executable.

All library records are stored in JSON format.

## Demo

See [dummy_data.txt](dummy_data.txt) for sample demonstration data.

## Technologies Used

- C++17
- Object-Oriented Programming
- File Handling
- JSON Data Storage
- STL

## Project Structure

```text
LibraryManagementsystem/
├── main.cpp
├── README.md
├── dummy_data.txt
├── .gitignore
└── library_data.json (generated at runtime)
```

## Author

Developed as a Library Management System project.
