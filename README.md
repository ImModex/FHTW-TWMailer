# FHTW-TWMailer (Spatz, Bogza)

## 1. Building the project
After downloading the source code, binaries have to be built using make.
- **make build** - builds full project
- make debug - builds project with debug info (e.g. for memory checks)
- make build-server - builds the server
- make build-client - builds the client
- make clean - removes all generated binaries

## 2. Starting the server
Navigate to the sub-directory ./server and run the following command: `./server <port>` \
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