# Task 1: TCP Chat

## Description
A simple multi-threaded TCP Chat Server and Client using Windows Sockets (Winsock2).

- **Server**: Listens on port 8888. Accepts multiple clients. Broadcasts received messages to all other connected clients.
- **Client**: Connects to localhost:8888. Sends user input to server and prints messages received from server.

## Build and Run

### Prerequisites
- MinGW-w64
- Wine

### Using Script
```bash
./run.sh
```
This will build the project, start the server in the background, and launch one client in the current terminal.
To test with multiple clients, open a new terminal, navigate to this directory, and run:
```bash
wine chat_client.exe
```

### Manual Build
```bash
make
# Terminal 1
wine chat_server.exe
# Terminal 2
wine chat_client.exe
```
