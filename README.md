# NexusPrime

NexusPrime is a lightweight, in-memory SQL-like database engine written in C++, featuring:

- **B+ Tree Indexes** for efficient primary‑key lookups  
- Support for **CREATE**, **INSERT**, **SELECT**, **UPDATE**, and **DELETE** SQL statements  
- **Range** and **exact** searches via B+ Tree  
- **Inner JOIN** across two tables, with optional `WHERE` filtering  
- Simple **SQL parser** that handles quoted strings and basic logical operators (`AND`/`OR`)  
- **Execution timing** printed in microseconds for each query  

---

## Table of Contents

1. [Features](#features)  
2. [Prerequisites](#prerequisites)  
3. [Building](#building)  
4. [Usage](#usage)  

---

## Features

- **B+ Tree implementation** (order 4) with leaf chaining for fast range scans  
- **Dynamic schema**: define tables and columns at runtime  
- **Index-backed INSERT**: enforces unique primary keys  
- **Range search**: `SELECT … WHERE key BETWEEN a AND b` uses the B+ Tree directly  
- **JOINs**: simple nested‑loop inner joins across two tables  
- **Automatic formatting** of query results in aligned columns  
- **Performance metrics**: each query reports its execution time  

---

## Prerequisites

- A C++17‑capable compiler (GCC, Clang, MSVC)  
- **Make** utility  

---

## Building

Use the provided Makefile to compile the project:

```bash
# macOS/Linux
make all
```

## Usage

Run the executable:

```bash
./bin/NexusPrime


