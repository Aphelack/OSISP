#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8888
#define BUFFER_SIZE 1024

DWORD WINAPI ReceiveHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        printf("\rServer: %s\nYou: ", buffer);
        fflush(stdout);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection failed.\n");
        return 1;
    }

    printf("Connected to server.\n");

    CreateThread(NULL, 0, ReceiveHandler, (LPVOID)clientSocket, 0, NULL);

    while (1) {
        printf("You: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        if (send(clientSocket, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            printf("Send failed.\n");
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
