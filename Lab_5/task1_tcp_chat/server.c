#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

SOCKET clients[MAX_CLIENTS];
HANDLE clientThreads[MAX_CLIENTS];
int clientCount = 0;
CRITICAL_SECTION clientLock;

void broadcastMessage(const char* message, SOCKET sender) {
    EnterCriticalSection(&clientLock);
    for (int i = 0; i < clientCount; i++) {
        if (clients[i] != sender) {
            send(clients[i], message, strlen(message), 0);
        }
    }
    LeaveCriticalSection(&clientLock);
}

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        printf("Received: %s\n", buffer);
        broadcastMessage(buffer, clientSocket);
    }

    EnterCriticalSection(&clientLock);
    for (int i = 0; i < clientCount; i++) {
        if (clients[i] == clientSocket) {
            clients[i] = clients[clientCount - 1];
            clientCount--;
            break;
        }
    }
    LeaveCriticalSection(&clientLock);

    closesocket(clientSocket);
    printf("Client disconnected.\n");
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    InitializeCriticalSection(&clientLock);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        return 1;
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

        printf("New client connected.\n");

        EnterCriticalSection(&clientLock);
        if (clientCount < MAX_CLIENTS) {
            clients[clientCount++] = clientSocket;
            CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
        } else {
            printf("Max clients reached. Connection rejected.\n");
            closesocket(clientSocket);
        }
        LeaveCriticalSection(&clientLock);
    }

    closesocket(serverSocket);
    WSACleanup();
    DeleteCriticalSection(&clientLock);
    return 0;
}
