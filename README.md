# WinApi-socket-realtime-cached-chat
This is a realtime chat that uses WinApi to manage sockets and create tcp connection between server and client. Also implements a caching system that save the messages between two peers.

The avaiable executables can make connection in localhost. If you want to change ip, you just have to change the ip from client side sourcecode(server is listening for all interfaces)

You have the possibility also to write message in offline mode(from client side) or from server side(when the client quit the session). When the connection is re-established the peers will send the new messages each other.

For this current version the client can support only one server caching.
