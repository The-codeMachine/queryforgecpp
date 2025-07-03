# QueryForge++

A modern, efficient C++ MySQL query connection pool built on the official MySQL Connector/C++.

## Features

- Thread-safe query queue and worker pool
- Automatic connection retry and recovery
- Prepared statement caching per worker thread
- Easy to use asynchronous query interface via `std::future`
- Cross-platform compatible (Windows, Linux, macOS)

## Getting Started

### Prerequisites

- C++17 compatible compiler
- MySQL Connector/C++ installed
- CMake (3.15 or higher)

### Installation

Clone the repository:

```bash
git clone https://github.com/The-codeMachine/queryforgecpp
cd QueryForge
mkdir build && cd build
cmake ..
make
```
Acknowledgments
If you find this project useful, a simple mention or recognition would be greatly appreciated, but it's not required. Thank you for using QueryForge++!
