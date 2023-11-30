#ifndef SERVICE_H
#define SERVICE_H
#define BUFSIZE 264
#include <winsock2.h>
#include <iostream>
#include <condition_variable>
#include <mutex>

using namespace std;

namespace srv
{
    void handle_new_messages(char newmessages[BUFSIZE], bool &notified, string chatbuffer, string &my_new_messages, condition_variable &cv, mutex &m1);
    void handle_message(char newmessages[BUFSIZE], string chatbuffer, string &my_new_messages, mutex &m1, string input);
    void handle_disconnect_message(string chatbuffer, string &my_new_messages, mutex &m1, string input, int &connection_flag, char *message);
    void client_listen_reicvmessage(SOCKET local_socket, int &connection_flag, const string chatbuffer, string &my_new_messages, mutex &m1, string &servername, condition_variable &cv, bool &notified, string &input);
    void server_listen_reicvmessage(SOCKET acceptedSocket, int &connection_flag, string &chatbuffer, string &my_new_messages, mutex &m1, string &clientname, condition_variable &cv, bool &notified, string &input);

    enum typelist
    {
        AUTH,
        MESSAGE,
        NEW_MESSAGE
    };

    enum connection_type_lis
    {
        CONNECT_WITH_NEW_MESSAGE,
        CONNECT,
        DISCONNECT
    };

    struct message
    {
        u_short type;
        char text[BUFSIZE];
    };
}

#endif