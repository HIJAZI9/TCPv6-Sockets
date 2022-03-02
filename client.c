
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include "client.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define STRICMP _stricmp

#define DEFAULT_SERVER     NULL // Will use the loopback interface
#define DEFAULT_FAMILY     PF_UNSPEC    // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE   SOCK_STREAM  // TCP
#define DEFAULT_PORT       "50000"
#define DEFAULT_EXTRA      0    // Number of "extra" bytes to send

#define BUFFER_SIZE        sizeof(_message_format)

#define UNKNOWN_NAME "<unknown>"


LPTSTR PrintError(int ErrorCode) {
    static TCHAR Message[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK,
                  NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  Message, 1024, NULL);
    return Message;
}

int ReceiveAndPrint(SOCKET ConnSocket, char *Buffer, int BufLen) {
    int AmountRead;
    _message_format rx_message;


    AmountRead = recv(ConnSocket, (char*) & rx_message, sizeof(_message_format), 0);
    if (AmountRead == SOCKET_ERROR) {
        fprintf(stderr, "recv() failed with error %d: %ls\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
        closesocket(ConnSocket);
        WSACleanup();
        exit(1);
    }


    if (AmountRead == 0) {
        printf("Server closed connection\n");
        closesocket(ConnSocket);
        WSACleanup();
        exit(0);
    }

    rx_message.student_number[6] = '\0';
    rx_message.message[1023] = '\0';
    rx_message.student_number[strcspn(rx_message.student_number, "\r\n")] = 0;
    rx_message.student_number[strcspn(rx_message.student_number, "\r\n")] = 0;

    printf("%s >> %s\n", rx_message.student_number, rx_message.message);

    return AmountRead;
}

int main(int argc, char **argv) {

    char Buffer[BUFFER_SIZE], AddrName[NI_MAXHOST];
    _message_format message;

    char *Server = DEFAULT_SERVER;
    int Family = DEFAULT_FAMILY;
    int SocketType = DEFAULT_SOCKTYPE;
    char *Port = (char *) DEFAULT_PORT;
    char* studentNumber; 

    WSADATA wsaData;

    int i, RetVal, AddrLen, AmountToSend;
    int ExtraBytes = DEFAULT_EXTRA;
    unsigned int MaxIterations = 1;
    BOOL RunForever = FALSE;

    ADDRINFO Hints, *AddrInfo, *AI;
    SOCKET ConnSocket = INVALID_SOCKET;
    struct sockaddr_storage Addr;

    struct fd_set readfds, writefds;

    struct timeval select_wait = {1, 0};

    Family = PF_INET6;
    SocketType = SOCK_STREAM;
    Server = argv[1];
    Port = argv[2];
    studentNumber = argv[3];
    
    // Ask for Winsock version 2.2.
    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed with error %d: %ls\n",
                RetVal, PrintError(RetVal));
        WSACleanup();
        return -1;
    }


    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = Family;
    Hints.ai_socktype = SocketType;
    RetVal = getaddrinfo(Server, Port, &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr,
                "Cannot resolve address [%s] and port [%s], error %d: %ls\n",
                Server, Port, RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // Try each address getaddrinfo returned, until we find one to which
    // we can successfully connect.
    //
    for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

        // Open a socket with the correct address family for this address.
        ConnSocket = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);

        printf("socket call with family: %d socktype: %d, protocol: %d\n",
               AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (ConnSocket == INVALID_SOCKET)
            printf("socket call failed with %d\n", WSAGetLastError());


        if (ConnSocket == INVALID_SOCKET) {
            fprintf(stderr, "Error Opening socket, error %d: %ls\n",
                    WSAGetLastError(), PrintError(WSAGetLastError()));
            continue;
        }

        printf("Attempting to connect to: %s\n", Server ? Server : "localhost");
        if (connect(ConnSocket, AI->ai_addr, (int) AI->ai_addrlen) != SOCKET_ERROR)
            break;

        i = WSAGetLastError();
        if (getnameinfo(AI->ai_addr, (int) AI->ai_addrlen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy_s(AddrName, sizeof(AddrName), UNKNOWN_NAME);
        fprintf(stderr, "connect() to %s failed with error %d: %ls\n",
                AddrName, i, PrintError(i));
        closesocket(ConnSocket);
    }

    if (AI == NULL) {
        fprintf(stderr, "Fatal error: unable to connect to the server.\n");
        WSACleanup();
        return -1;
    }
    //
    // Check where a socket is connected.
    //
    AddrLen = sizeof(Addr);
    if (getpeername(ConnSocket, (LPSOCKADDR) &Addr, (int *) &AddrLen) == SOCKET_ERROR) {
        fprintf(stderr, "getpeername() failed with error %d: %ls\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
    } else {
        if (getnameinfo((LPSOCKADDR) &Addr, AddrLen, AddrName, sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy_s(AddrName, sizeof(AddrName), UNKNOWN_NAME);
        printf("Connected to %s, port %d, protocol %s, protocol family %s\n",
               AddrName, ntohs(SS_PORT(&Addr)),
               (AI->ai_socktype == SOCK_STREAM) ? "TCP" : "UDP",
               (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6");
    }

    // No more requirmenent for address info chain, so we can free it.
    freeaddrinfo(AddrInfo);

    //
    //print local address and port the system picked for us.
    //
    AddrLen = sizeof(Addr);
    if (getsockname(ConnSocket, (LPSOCKADDR) &Addr, &AddrLen) == SOCKET_ERROR) {
        fprintf(stderr, "getsockname() failed with error %d: %ls\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
    } else {
        if (getnameinfo((LPSOCKADDR) &Addr, AddrLen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy_s(AddrName, sizeof(AddrName), UNKNOWN_NAME);
        printf("Using local address %s, port %d\n",
               AddrName, ntohs(SS_PORT(&Addr)));
    }

    /* Start communicating with the peer */
    for (;;) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        // FD_ZERO() löscht einen Satz
        FD_SET(ConnSocket, &readfds);

        int retval = select(0, &readfds, NULL, NULL, &select_wait);
        if (retval == SOCKET_ERROR) {
            fprintf(stderr, "select failed %d\n", WSAGetLastError());
            exit(0);
        }

        if ((ConnSocket != INVALID_SOCKET) && (FD_ISSET(ConnSocket, &readfds))) {
            //FD_ISSET --> Testen eines bestimmten Sockets auf Lesbarkeit unter Verwendung des readfds-Parameters
            // Clear buffer just to prove we're really receiving something.
            memset(Buffer, 0, sizeof(Buffer));

            // Receive and print server's reply.
            ReceiveAndPrint(ConnSocket, Buffer, sizeof(Buffer));
        } else {
            printf(" Enter message to be sent \n");
            memset(&message, 0x00, sizeof(message));
            fgets(message.message, (unsigned)_countof(message.message) - 1, stdin);
            message.message[strcspn(message.message, "\r\n")] = 0;
            memcpy_s(message.student_number, sizeof(message.student_number), studentNumber, sizeof(message.student_number));
            message.student_number[6] = '\0';

            // Compose a message to send.
            AmountToSend = sprintf_s(Buffer, (unsigned) _countof(message.message), "%s %s", message.student_number, message.message);

           /* sizeof(message.message) = 40 bytes
                _countof(message.message) = 1024 elements*/


            // Send the message.  Since we are using a blocking socket, this
            // call shouldn't return until it's able to send the entire amount.
            RetVal = send(ConnSocket, (char *) & message, sizeof(_message_format), 0);
            if (RetVal == SOCKET_ERROR) {
                fprintf(stderr, "send() failed with error %d: %ls\n",
                        WSAGetLastError(), PrintError(WSAGetLastError()));
                WSACleanup();
                return -1;
            }

           // printf("Debug Info: Sent %d bytes (out of %d bytes) of data: [%.*s]\n",
                //   RetVal, AmountToSend, AmountToSend, Buffer);
            printf("%s >> %s\n", message.student_number, message.message);
        }
    }


    // We are done communicating with the client
    printf("Done sending\n");
    shutdown(ConnSocket, SD_SEND);

    //
    // Receive the final messgage if any and then close the connection
    //
    if (SocketType == SOCK_STREAM)
        while (ReceiveAndPrint(ConnSocket, Buffer, sizeof(Buffer)) != 0);

    closesocket(ConnSocket);
    WSACleanup();
    return 0;
}