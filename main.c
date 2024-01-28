#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 1024
#define ADDRESS "127.0.0.1"
#define PORT 8080

void cleanup(SOCKET listener);
int readFile(const char *filename, char **output);

int main() {
    printf("hey!\n");

    int res, sendRes;
    int running;
    WSADATA wsaData;
    SOCKET listener, client;
    struct sockaddr_in address, clientAddr;
    char recvbuf[BUFLEN];
    char *inputFileContents;
    int inputFileLength;

    //initialization

    res = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (res) {
        printf("Startup failed\n");
        return 1;
    }

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        printf("Error with construction\n");
        cleanup(0);
        return 1;
    }

    //bind server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);
    res = bind(listener, (struct sockaddr *)&address, sizeof(address));
    if (res == SOCKET_ERROR) {
        printf("Bind failed\n");
        cleanup(listener);
        return 1;
    }

    res = listen(listener, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        printf("Listen failed\n");
        cleanup(listener);
        return 1;
    }

    // load file
    inputFileLength = readFile("index.html", &inputFileContents);
    if(!inputFileLength || !inputFileContents) {
        printf("Could not read html file\n");
        cleanup(listener);
        return 1;
    }

    printf("%s\n", inputFileContents);

    // done setting up
    printf("Accepting on %s:%d\n", ADDRESS, PORT);
    while (running) {
        // accept client
        int clientAddrLen;
        client = accept(listener, NULL, NULL);
        if(client == INVALID_SOCKET) {
            printf("Could not accept\n");
            cleanup(listener);
            return 1;
        }

        //get client info
        getpeername(client, (struct sockaddr *)&clientAddr, &clientAddrLen);
        printf("Client connect at: %s:%d\n", inet_ntoa(address.sin_addr));

        //receive
        res = recv(client, recvbuf, BUFLEN, 0);
        if ( res > 0) {
            recvbuf[res] = 0;
            // printf("%s\n", recvbuf);

            // test if  GET request

        if (!memcmp(recvbuf, "GET", 3)) {
            printf("GET\n");
            sendRes = send(client, inputFileContents, inputFileLength, 0);

            if(sendRes == SOCKET_ERROR) {
                printf("Send failed\n");
            }
        }

        
        if (!memcmp(recvbuf, "POST", 4)) {
            printf("POST\n");
            
            if(sendRes == SOCKET_ERROR) {
                printf("Send failed\n");
            }
        }

        } else if (!res) {
            printf("Client disconnected!\n");
        } else {
            printf("Receive failed\n");
        }

        shutdown(client, SD_BOTH);
        closesocket(client);
        client = INVALID_SOCKET;
    }

    cleanup(listener);
    printf("Shutting down.\n");

    return 0;
}

void cleanup(SOCKET listener) {
    if (listener && listener != INVALID_SOCKET) {
        closesocket(listener);
    }

    WSACleanup();
}

int readFile(const char *filename, char **output) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }

    // get length
    // move cursor to the end
    fseek(fp, 0L, SEEK_END);
    // get remaining length
    int len = ftell(fp);
    // return to original position
    fseek(fp, 0, SEEK_SET);

    // read
    *output = malloc(len + 1);
    fread(*output, len, 1, fp);
    (*output)[len] = 0;
    fclose(fp);

    return len;
}