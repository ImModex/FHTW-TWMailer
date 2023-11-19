# FHTW-TWMailer (Spatz, Bogza)

## 1. Building the project
After downloading the source code, binaries have to be built using make.
To compile this project we used gcc version 11.4
- **make build** - builds full project
- make debug - builds project with debug info (e.g. for memory checks)
- make build-server - builds the server
- make build-client - builds the client
- make clean - removes all generated binaries

## 2. Starting the server
Navigate to the sub-directory ./server and run the following command: `./server <port> <mail-spool-directory>` \
After starting the server, clients can connect to it using the port given.

## 3. Starting the/a client
Navigate to the sub-directory ./client and run the following command: `./client <ip> <port>` \
If the server is running on your local machine you can enter 'localhost' or '127.0.0.1' for IP. \
The server supports multiple clients connected at once.

## 4. Client Usage
After the server has been started and a client has connected, there are a series of available commands. \
(Attention: Commands are case-sensitive!)

### 4.1 LOGIN
Used to log into a user account, passwords are not implemented yet (preparation for TWMailer Pro - LDAP Login) \
For every following command you **must be logged in**!!! \
Usage:
```
LOGIN
<Username>
<Password>
```

Example Answer:
```
OK
```

### 4.2 SEND
Send an E-Mail \
Usage:
```
SEND
<Receiver>
<Subject>
<Message (multi-line)>
.
```

Example Answer:
```
OK
```

### 4.3 LIST
Display a list of E-Mails in your inbox \
Usage:
```
LIST
```

Example Answer:
```
OK
Amount of messages in mail directory
0: Subject_1
1: Subject_2
2: Subject_3
```

### 4.4 READ
Read an E-Mail \
Usage:
```
READ
<Index>
```

Example Answer:
```
OK
if22b140
Important Subject
Important Message
Other line of message
```

### 4.5 DEL
Delete an E-Mail \
Usage:
```
DEL
<Index>
```

Example Answer:
```
OK
```

### 4.6 QUIT
Exit the program \
Usage:
```
QUIT
```
No answer.

## 5. Code Structure
As the program grew, we split some code apart and built seperate library files for it (./lib); Server and Client code are in their respective directories.

### 5.1 Queue Library (./lib/queue.h)
Small library to create lists of constant-sized objects. \
Lists can be created, objects added, queried and freed.

### 5.2 Packet Library (./lib/tw_packet.h)
Structures the above mentioned E-Mail protocol into packets, handles IO and manages memory.

### 5.3 Utility Library (./lib/tw_utility.h)
Some general utility functions that we have accumulated over time and are nicely reusable. (altered to fit the TW-Mailer)

### 5.4 LDAP Library (./lib/myldap.h)
Handles LDAP Login for FHTW LDAP Server.

### 5.5 Hide Password Library (./lib/hide_pw.h)
Display asteriks characters when the user types in their password to login.

### 6. Server Client Architecture 
This project is a connection based, concurent, messaging programm.

### 7. Development strategies and needed protocol adaptations
The development strategy used was feature based development, since there was a clear guideline to follow it was intuitive to work on implementing requirements in order. \
Protocol adaptations were minimal, the ldapConnection method was changed to the string as input instead of an argument vector and the hide password functionality
was changed to C from C++

### 8. Used synchronization methods
To synchronize the application a mutex was implemented to avoid simultanious writing to files by multiple users

### 9. Handling of large messages 
Messages are being split into chunks of data before being sent, this process is repeated until the bytes sent equals the packet data size