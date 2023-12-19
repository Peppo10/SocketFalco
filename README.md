# Socket-realtime-cached-chat
This is a realtime chat that uses Win/Linux api to manage sockets and create tcp connection between server and client. Also implements a caching system that save the messages between two peers.

You have the possibility also to write message in offline mode when client or server quit the session. When the connection is re-established the peers will send the new messages each other.

# Running
From client side you must pass the server ip address by argument.

`client.exe <ip-address>`

From server side you can just run it.


## NOTE
* if you want to save the chat you MUST exit by typing "quit" on the console.

## DIAGRAM
![Diagramma senza titolo drawio (2)](https://github.com/Peppo10/Socket-realtime-cached-chat/assets/131891564/273fdd11-c3d1-43b9-b956-5c404040b58b)


