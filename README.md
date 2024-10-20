# SocketFalco
This is a realtime chat that uses Win/Linux api to manage sockets and create tcp connection between server and client. Also implements a caching system that save the messages between two peers.

You have the possibility also to write message in offline mode when client or server quit the session. When the connection is re-established the peers will send the new messages each other.

Every chat is unique and depends on the uuid of the device connected to. Does not matter who is the server or the client, the chat will be the same.

Every device at first start-up will generate his uuid4, which will be used for the authentication.

# Usage
  ```
SocketFalco -h   
Usage: SocketFalco [options] <arguments>

Options:
-l, -listen                   Start the server. Use -t for temporary session.
                                Example: SocketFalco -l
                                         SocketFalco -l -t

-ls, -list                    List available chat sessions.
                                Example: SocketFalco -ls

-c, -connect <address>        Connect to a specified server address. Use -t for temporary session.
                                Example: SocketFalco -c 192.168.1.1
                                         SocketFalco -c -t 192.168.1.1

-eN, -editName <name>         Edit the current user name.
                                Example: SocketFalco -eN NewName

-eC, -editChat <ChatUUID>     Open the specific chat
                                Example: SocketFalco -eC chatUUID

-h, -help                     Show this help message and exit.

Examples:
SocketFalco -listen                   Start the server.
SocketFalco -listen -t                Start the server in temporary mode.
SocketFalco -connect 192.168.0.1      Connect to the server at 192.168.0.1.
SocketFalco -connect -t 192.168.0.1   Connect to the server at 192.168.0.1 in temporary mode.
SocketFalco -list                     List all chat sessions.
SocketFalco -editName Alice           Change the username to "Alice".
SocketFalco -editChat 97b8b...        Open the specific chat and let the user to add new messages.
  ```


# Note
* If you want to save the chat you MUST exit by typing "quit" on the console.
* Server can support localhost connection, you're just chatting with your device(cache file is the same).

# Diagram
![Diagram](https://github.com/user-attachments/assets/1383960b-31d2-4db6-942d-a6fd0e104b4c)

# Data Message Format
![socket-realtime-cache-datagram drawio](https://github.com/Peppo10/Socket-realtime-cached-chat/assets/131891564/dc63afad-bd6e-49f8-a81e-bb42cea59b36)



