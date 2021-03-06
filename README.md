# TCPv6-Sockets
Chat-Programm ( TCPv6-Sockets )

Realization of a chat program based on TCPv6 sockets

Task : -->

Write down a sender (client) and a receiver (server) program in C (console programs)
Help of which text messages can be transmitted from the sender to the recipient via TCPv6 / IPv6.
The sender and receiver are both able to send and receive data.
The sender receives the IPv6 address and the port of the recipient as well as your s number
Pass identification as program arguments. The recipient should also use program arguments
configurable: the number of your communication partner and the port on which the recipient
should hear.

Implement the following functionalities in various C modules for sender and receiver:

1. Sender:

a. Initializing the sender by creating a TCP socket;

b. Call of select (). Depending on the return either continue with c. or d

c. Reading in the text specified via the console.

Structure of the packet to be sent (use a data structure e.g. struct packet (char
text[…]; … SNumber;}. Define the structure in a separate header file. It is
sufficient to accept a text with a maximum of 1024 characters.
Note: You can use this structure directly on the TCP socket function without serialization
send () passed!

d. Possible receipt of recv () a message from the communication partner: output of the
Identification (sNumber) and the received message on the console, e.g. like this:
s12345> Hello ...

2. receiver:

a. Initialize the tcp socket (don't forget the bind ());

b. Otherwise b.-d. like with the transmitter.

The functionality of the client and server does not differ significantly. Just the call
of bind and listen and thus the call sequence of the 2 application instances in the
They differ in execution. Start the server accordingly first so that the incoming
Connection can be heard by the client.

Notice:

Work without further threads! You can send and receive at the same time using the select () -
Realize function. This enables asynchronous sockets and allows you to distinguish whether
Data is available via the keyboard (stdin) or via the network (socket file descriptor). You can
then either send the data to the recipient or read the data from the socket.
Make sure you are using non-blocking functions for sending and receiving
Data work.

select () - socket function

ioctlsocket function (winsock2.h) - Win32 apps | Microsoft Docs


## Operation: Client (rnks_chat_client.exe)
--> put Client.c and Client.h in a file name ,which is (rnks_chat_client).

The client is called by specifying three mandatory arguments. If these are not given or only given incompletely, the program will be terminated with an error message. The call syntax follows the following principle (the order of the arguments cannot be chosen arbitrarily):

**Calling syntax:** `rnks_chat_client.exe <address> <port> <name>`

**Arguments:**
 argument --> Description 


 `<address>` --> IP address of the computer on which the server program is running 

 `<port>`    --> Port number via which the connection to the server is established 

 `<name>`    --> Username (e.g. library no. (Sxxxxx))   
 
 Open images Folder , then Argumente_Client.jpg -->  images/Argumente_Client.jpg
  
After a successful call, the client is initialized and begins to connect to the server (see screenshot below).

![connection passed](https://user-images.githubusercontent.com/72709664/156903822-92b041c9-7651-462b-a386-37d9f9e0ba9e.jpg)

After a connection is established between the server and the client, message exchange begins. For the client, it sends messages with the username: s82822 to the server. The client also receives messages from the server with username: s82802 and displays them on the screen (see screenshot below).

![client_send_recieve](https://user-images.githubusercontent.com/72709664/156903852-eacfa3e2-dcc8-444d-8424-c415bcfc1df6.jpg)

## Operation: Client (rnks_chat_server.exe)
--> put Server.c and Server.h in a file name ,which is (rnks_chat_client).

**Calling syntax:**
The server is called by specifying a mandatory argument. If these are not given or only given incompletely, the program will be terminated with an error message.

**Arguments:**
`<name>`    --> Username (e.g. library no. (Sxxxxx))    

 Open images Folder , then Argumente_Server.jpg -->  images/Argumente_Server.jpg

After a connection is established between the server and the client, message exchange begins. For the server, it sends messages with the username: s82802 to the client. The server also receives messages from the client with username: s82822 and displays them on the screen (see screenshot below).

![Server_Connection_send_revieve](https://user-images.githubusercontent.com/72709664/157330388-fe23e887-6ed4-4163-8f3d-c8d49f368d4a.jpg)
