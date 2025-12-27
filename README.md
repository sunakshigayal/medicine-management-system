# Medicine Management System (Terminal-Based)

## Description
This is a **terminal-based Medicine Management System** developed in C.
It allows users to manage medicine inventory via a console interface. Users can register, login, add/update/delete medicines, track expiry dates, and view stock status.

The system includes a **Staff/Admin user hierarchy**, where Admin accounts can manage the system fully, and Staff accounts have limited access.

---

## Features

### User Management
- Register new users (default role: STAFF)
- Login with username/password
- Staff/Admin selection after login
- Admin promotion using a secret key

### Medicine Management
- Add new medicine records
- Update medicine quantity
- Delete medicine records
- Search medicine by batch number
- Display all medicines in the system
- Automatic sorting by expiry date

### Stock & Expiry Tracking
- IN STOCK / LOW STOCK / OUT OF STOCK
- NEAR EXPIRY (less than 30 days)
- EXPIRED medicines detection
- Statistics report for inventory

### Menu Interface
- Staff Menu: View medicines, search, expiry tracking, stats
- Admin Menu: Full CRUD + stats + expiry tracker

---

## Technologies Used
- Programming Language: **C**
- Concepts:
  - Structures
  - Functions
  - Linked Lists
  - File Handling
  - Menu-driven programming
  - Time and date handling (expiry calculation)

---

## How to Run
1. Compile the program:
```bash
gcc medicine_tracker.c -o medicine_tracker
