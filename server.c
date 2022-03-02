
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "server.h"

// Needed for the Windows 2000 IPv6 Tech Preview.
#if (_WIN32_WINNT == 0x0500)
#include <tpipv6.h>
#endif

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define STRICMP _stricmp


#define DEFAULT_FAMILY     PF_UNSPEC    // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE   SOCK_STREAM  // TCP
#define DEFAULT_PORT       "50000"       // Arbitrary, albiet a historical test port
#define BUFFER_SIZE        sizeof(_message_format)   // Set very small for demonstration purposes


LPSTR PrintError(int ErrorCode)
{
    static char Message[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)Message, 1024, NULL);
    return Message;
}

int main(int argc, char** argv)
{
    char Buffer[BUFFER_SIZE], Hostname[NI_MAXHOST];
    int Family = DEFAULT_FAMILY;
    int SocketType = DEFAULT_SOCKTYPE;
    char* Port = (char*)DEFAULT_PORT;
    char* Address = NULL;
    int i, NumSocks, RetVal, FromLen, AmountRead, AmountToSend;
    //    int idx;
    SOCKADDR_STORAGE From;
    WSADATA wsaData;
    ADDRINFO Hints, * AddrInfo, * AI;
    SOCKET ServSock[FD_SETSIZE];
    fd_set SockSet;
    char* studentNumber = argv[1];

    
    // Ask for Winsock version 2.2.
    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed with error %d: %s\n",
            RetVal, PrintError(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to bind
    // to a socket(s) for accepting incoming connections.  This means that
    // when the Address parameter is NULL, getaddrinfo will return one
    // entry per allowed protocol family containing the unspecified address
    // for that family.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = Family;
    Hints.ai_socktype = SocketType;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    RetVal = getaddrinfo(Address, Port, &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr, "getaddrinfo failed with error %d: %s\n",
            RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }
    //
    // For each address getaddrinfo returned, we create a new socket,
    // bind that address to it, and create a queue to listen on.
    //
    for (i = 0, AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

        // Highly unlikely, but check anyway.
        if (i == FD_SETSIZE) {
            printf("getaddrinfo returned more addresses than we could use.\n");
            break;
        }

        if ((AI->ai_family != PF_INET) && (AI->ai_family != PF_INET6))
            continue;

        // Open a socket with the correct address family for this address.
        ServSock[i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (ServSock[i] == INVALID_SOCKET) {
            fprintf(stderr, "socket() failed with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            continue;
        }

        if ((AI->ai_family == PF_INET6) &&
            IN6_IS_ADDR_LINKLOCAL((IN6_ADDR*)INETADDR_ADDRESS(AI->ai_addr)) &&
            (((SOCKADDR_IN6*)(AI->ai_addr))->sin6_scope_id == 0)
            ) {
            fprintf(stderr,
                "IPv6 link local addresses should specify a scope ID!\n");
        }

        //
        // bind() associates a local address and port combination
        // with the socket just created. This is most useful when
        // the application is a server that has a well-known port
        // that clients know about in advance.
        //
        if (bind(ServSock[i], AI->ai_addr, (int)AI->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr, "bind() failed with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            closesocket(ServSock[i]);
            continue;
        }

        if (SocketType == SOCK_STREAM) {
            if (listen(ServSock[i], 5) == SOCKET_ERROR) {
                fprintf(stderr, "listen() failed with error %d: %s\n",
                    WSAGetLastError(), PrintError(WSAGetLastError()));
                closesocket(ServSock[i]);
                continue;
            }
        }

        printf("'Listening' on port %s, protocol %s, protocol family %s\n",
            Port, (SocketType == SOCK_STREAM) ? "TCP" : "UDP",
            (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6");
        i++;
    }

    freeaddrinfo(AddrInfo);

    if (i == 0) {
        fprintf(stderr, "Fatal error: unable to serve on any address.\n");
        WSACleanup();
        return -1;
    }
    NumSocks = i;

    //
    // We now put the server into an eternal loop,
    // serving requests as they arrive.
    //
    FD_ZERO(&SockSet);
    while (1) {

        FromLen = sizeof(From);

        for (i = 0; i < NumSocks; i++) {
            if (FD_ISSET(ServSock[i], &SockSet))
                break;
        }
        if (i == NumSocks) {
            for (i = 0; i < NumSocks; i++)
                FD_SET(ServSock[i], &SockSet);
            if (select(NumSocks, &SockSet, 0, 0, 0) == SOCKET_ERROR) {
                fprintf(stderr, "select() failed with error %d: %s\n",
                    WSAGetLastError(), PrintError(WSAGetLastError()));
                WSACleanup();
                return -1;
            }
        }
        for (i = 0; i < NumSocks; i++) {
            if (FD_ISSET(ServSock[i], &SockSet)) {
                FD_CLR(ServSock[i], &SockSet);
                break;
            }
        }

        if (SocketType == SOCK_STREAM) {
            SOCKET ConnSock;

            //
            // Since this socket was returned by the select(), we know we
            // have a connection waiting and that this accept() won't block.
            //
            ConnSock = accept(ServSock[i], (LPSOCKADDR)&From, &FromLen);
            if (ConnSock == INVALID_SOCKET) {
                fprintf(stderr, "accept() failed with error %d: %s\n",
                    WSAGetLastError(), PrintError(WSAGetLastError()));
                WSACleanup();
                return -1;
            }
            if (getnameinfo((LPSOCKADDR)&From, FromLen, Hostname,
                sizeof(Hostname), NULL, 0, NI_NUMERICHOST) != 0)
                strcpy_s(Hostname, NI_MAXHOST, "<unknown>");
            printf("\nAccepted connection from %s\n", Hostname);


            while (1) {
                memset(Buffer, 0x00, BUFFER_SIZE);
                _message_format rx_message, tx_message;

                memset(&tx_message, 0x00, sizeof(_message_format));
                memset(&rx_message, 0x00, sizeof(_message_format));
                AmountRead = recv(ConnSock, &rx_message, sizeof(_message_format), 0);
                if (AmountRead == SOCKET_ERROR) {
                    fprintf(stderr, "recv() failed with error %d: %s\n",
                        WSAGetLastError(), PrintError(WSAGetLastError()));
                    closesocket(ConnSock);
                    break;
                }
                if (AmountRead == 0) {
                    printf("Client closed connection\n");
                    closesocket(ConnSock);
                    break;
                }

                rx_message.student_number[6] = '\0';
                rx_message.message[1023] = '\0';
                rx_message.student_number[strcspn(rx_message.student_number, "\r\n")] = 0;
                rx_message.student_number[strcspn(rx_message.student_number, "\r\n")] = 0;
                printf("%s >> %s\n", rx_message.student_number, rx_message.message);

                printf("Enter message to be sent: ");
                fgets(tx_message.message, (unsigned)_countof(tx_message.message) - 1, stdin);
                tx_message.message[strcspn(tx_message.message, "\r\n")] = 0;
                memcpy_s(tx_message.student_number, sizeof(tx_message.student_number), studentNumber, sizeof(tx_message.student_number));
                tx_message.student_number[6] = '\0';

                // Compose a message to send.
                AmountToSend = sprintf_s(Buffer, (unsigned)_countof(tx_message.message), "%s %s", tx_message.student_number, tx_message.message);

                RetVal = send(ConnSock, &tx_message, sizeof(_message_format), 0);
                if (RetVal == SOCKET_ERROR) {
                    fprintf(stderr, "send() failed: error %d: %s\n",
                        WSAGetLastError(), PrintError(WSAGetLastError()));
                    closesocket(ConnSock);
                    break;
                }
            }

        }
    }

    return 0;
}