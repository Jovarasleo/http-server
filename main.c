#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")


int readConfigFile(const char *filename, int defaultPort) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open config file: %s. Using default port: %d\n", filename, defaultPort);
        return defaultPort;
    }

    int port;
    if (fscanf(file, "port=%d", &port) != 1) {
        printf("Error reading port from config file. Using default port: %d\n", defaultPort);
        fclose(file);
        return defaultPort;
    }

    fclose(file);
    return port;
}

void handleClient(SOCKET clientSocket) {
    FILE *htmlFile = fopen("index.html", "r");
    if (htmlFile == NULL) {
        printf("Failed to open HTML file\n");
        closesocket(clientSocket);
        return;
    }

    fseek(htmlFile, 0, SEEK_END);
    long contentLength = ftell(htmlFile);
    fseek(htmlFile, 0, SEEK_SET);

    char responseHeader[256];
    snprintf(responseHeader, sizeof(responseHeader), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", contentLength);

    // Debugging statement
    printf("Sending response header:\n%s\n", responseHeader);

    // Send the response header
    int bytesSent = send(clientSocket, responseHeader, strlen(responseHeader), 0);
    if (bytesSent == SOCKET_ERROR) {
        printf("Error sending response header: %d\n", WSAGetLastError());
        fclose(htmlFile);
        closesocket(clientSocket);
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    size_t totalBytesSent = 0;

    // Send the entire HTML content at once
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), htmlFile)) > 0) {
        int bytesSent = send(clientSocket, buffer, bytesRead, 0);
        if (bytesSent == SOCKET_ERROR) {
            printf("Error sending HTML content: %d\n", WSAGetLastError());
            fclose(htmlFile);
            closesocket(clientSocket);
            return;
        }

        totalBytesSent += bytesSent;
    }

    // Debugging statement
    printf("Total bytes sent: %zu\n", totalBytesSent);

    fclose(htmlFile);

    // Close the client socket after sending the content
    closesocket(clientSocket);

    // Debugging statement
    printf("Client socket closed\n");
}

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        WSACleanup();
        return 1;
    }

    // Read configuration from file
    int PORT = readConfigFile("config.txt", 3000);

    // Set up the server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Listen on localhost
    serverAddr.sin_port = htons(PORT);  // Listen on port 7000

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error code: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on localhost:%d",PORT,"\n");

    // Accept incoming connections and print information
    while (1) {
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed with error code: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        // Read and send HTML content
        handleClient(clientSocket);
    }

    // Clean up
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}