/*MIT License

Copyright (c) 2023 Peppo10

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef SERVICE_H
#define SERVICE_H
#define BUFSIZE 264

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET _SOCKET;
#else
#include <sys/socket.h>
typedef int _SOCKET;
#endif

#include <string.h>
#include <iostream>
#include <condition_variable>
#include <mutex>

using namespace std;

namespace srv
{
    void handle_new_messages(char newmessages[BUFSIZE], bool &notified, string chatbuffer, string &my_new_messages, condition_variable &cv, mutex &m1);
    void handle_message(char newmessages[BUFSIZE], string chatbuffer, string &my_new_messages, mutex &m1, string input);
    void handle_disconnect_message(string chatbuffer, string &my_new_messages, mutex &m1, string input, int &connection_flag, char *message);
    void client_listen_reicvmessage(_SOCKET local_socket, int &connection_flag, const string chatbuffer, string &my_new_messages, mutex &m1, string &servername, condition_variable &cv, bool &notified, string &input);
    void server_listen_reicvmessage(_SOCKET acceptedSocket, int &connection_flag, string &chatbuffer, string &my_new_messages, mutex &m1, string &clientname, condition_variable &cv, bool &notified, string &input);

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