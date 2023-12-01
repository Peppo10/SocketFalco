# WinApi-socket-realtime-cached-chat
This is a realtime chat that uses WinApi to manage sockets and create tcp connection between server and client. Also implements a caching system that save the messages between two peers.

You have the possibility also to write message in offline mode(from client side) or from server side(when the client quit the session). When the connection is re-established the peers will send the new messages each other.

The avaiable executables can make connection in localhost. If you want to change ip, you just have to change the ip from client side sourcecode(server is listening for all interfaces)

For this current version the client can support only one server caching.

NOTE: if you want to save the chat you MUST exit by typing "quit" on the console.

# DIAGRAM
![Diagramma senza titolo drawio](https://github.com/Peppo10/WinApi-socket-realtime-cached-chat/assets/131891564/951c5e3a-8a11-4025-b210-67f746cfb683)
