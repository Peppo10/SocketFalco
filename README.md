# Socket-realtime-cached-chat
This is a realtime chat that uses Win/Linux api to manage sockets and create tcp connection between server and client. Also implements a caching system that save the messages between two peers.

You have the possibility also to write message in offline mode when client or server quit the session. When the connection is re-established the peers will send the new messages each other.

Every chat is unique and depends on the uuid of the device connected to. Does not matter who is the server or the client, the chat will be the same.

Every device at first start-up will generate his uuid4, wich will be used for the authentication.

# Running
* From client side you must pass the server ip address by argument.

  `client.exe <ip-address>`

* From server side you can just run it.


# Note
* If you want to save the chat you MUST exit by typing "quit" on the console.
* Server can support localhost connection, you're just chatting with your device(cache file is the same).

# Diagram
![Diagramma senza titolo drawio (2)](https://github.com/Peppo10/Socket-realtime-cached-chat/assets/131891564/273fdd11-c3d1-43b9-b956-5c404040b58b)

# Data Message Format
![socket-realtime-cache-datagram drawio](https://github.com/Peppo10/Socket-realtime-cached-chat/assets/131891564/dc63afad-bd6e-49f8-a81e-bb42cea59b36)



