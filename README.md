# Secure Multi-Client Network Monitoring System using TCP (C++)

A beginner-friendly academic **TCP client-server** project where multiple clients send simulated monitoring data to a server.  
The server authenticates clients, prints received data, generates simple alerts, and writes logs to `logs.txt`.

## Project Title & Description
**Title:** Secure Multi-Client Network Monitoring System using TCP (C++)  
**Description:** A terminal-based TCP server on port **8080** that handles **multiple clients concurrently** using threads. Clients send data like `CPU:80;LOGIN_FAIL:2`. The server checks thresholds and generates alerts.

## Features
- **TCP client-server model** with sockets
- **Multi-client support** using `std::thread` (concurrent server)
- **Simple authentication** (`AUTH:1234`) before accepting data
- **Alert generation**
  - `CPU > 80` → `High CPU Alert`
  - `LOGIN_FAIL > 5` → `Brute Force Alert`
- **File logging**: appends all events to `logs.txt`

## Architecture (client-server flow)
```text
Client                     Server (port 8080)
  |  connect()  -------------------->  accept()
  |  "AUTH:1234\n" ---------------->  verify auth
  |  <------ "OK\n" (if auth valid)
  |  "CPU:..;LOGIN_FAIL:..\n" ----->  parse + alert + log
  |  close()  --------------------->  disconnect + log
```

## Tech Stack
| Category | Used |
|---|---|
| Language | C++ |
| Networking | TCP sockets (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`) |
| Concurrency | `std::thread` |
| Logging | `std::ofstream` (`logs.txt`) |
| Build | `Makefile` |

## How It Works
- **Server (`server.cpp`)**
  - Listens on **port 8080**
  - Spawns a **new thread** per client after `accept()`
  - Requires first message: `AUTH:1234`
  - Receives data lines and parses `CPU` and `LOGIN_FAIL`
  - Prints alerts and appends logs to `logs.txt`
- **Client (`client.cpp`)**
  - Connects to `127.0.0.1:8080` by default
  - Sends auth, then sends a few **simulated random** data lines

## Setup & Run Instructions
**Prerequisites:** `g++` and `make` (Linux/WSL recommended)

> **Windows note:** This project uses POSIX socket headers (e.g., `sys/socket.h`). Use **WSL (Ubuntu)** or a Linux environment.

From the `project/` folder:

```bash
make all
```

Run the server (Terminal 1):

```bash
./server
```

Run one or more clients (Terminal 2, Terminal 3, ...):

```bash
./client
```

Optional: custom IP / port / key:

```bash
./client 127.0.0.1 8080 1234
```

Clean build outputs:

```bash
make clean
```

## Example Input/Output
**Example client message format (sent to server):**
```text
CPU:80;LOGIN_FAIL:2
```

**Server output (example):**
```text
Server started on port 8080
Waiting for clients...

2026-03-31 21:40:10 | CONNECTED | 127.0.0.1:53122
2026-03-31 21:40:10 | DATA | 127.0.0.1:53122 | CPU:85;LOGIN_FAIL:2
2026-03-31 21:40:10 | ALERT | 127.0.0.1:53122 | High CPU Alert (CPU=85)
2026-03-31 21:40:11 | DATA | 127.0.0.1:53122 | CPU:60;LOGIN_FAIL:7
2026-03-31 21:40:11 | ALERT | 127.0.0.1:53122 | Brute Force Alert (LOGIN_FAIL=7)
```

**Client output (example):**
```text
Authenticated. Sending monitoring data...
Done. Closing connection.
```

## Folder Structure
```text
project/
├─ server.cpp
├─ client.cpp
├─ Makefile
├─ logs.txt
└─ README.md
```

## Future Improvements
- Configurable auth key and thresholds via command-line arguments
- Better message validation (handle missing/invalid fields cleanly)
- Add graceful shutdown (signal handling)
- Add client ID / hostname in logs

## Author
- **Author:** Avikal Singh
- **Course/Activity:** Computer Networks (TCP Socket Programming Lab)