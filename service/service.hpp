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

#include <string.h>
#include <condition_variable>
#include <mutex>
#include <cstdarg>
#include "../caching/caching.hpp"


#ifdef _WIN32
#define _CLOSE_SOCKET closesocket
#elif __linux__
#define _CLOSE_SOCKET close
#endif

using namespace std;

namespace srv
{
    enum connection_type_list
    {
        CONNECT_WITH_NEW_MESSAGE,
        CONNECT,
        DISCONNECT
    };

    
    void handle_new_messages(clca::msg::Message newMessage, bool &notified, clca::Chat &chat, condition_variable &cv, mutex &m1, int *newmsg);
    
    void handle_message(clca::msg::Message newMessage, clca::Chat &chat, mutex &m1, string input);
    
    void handle_disconnect_message(clca::Chat &chat, mutex &m1, string input, int &connection_flag, char *message);
    
    void client_listen_reicvmessage(_SOCKET local_socket, int &connection_flag, clca::Chat &chat, mutex &m1, basic_string<_PATH_CHAR> &serveruuid, condition_variable &cv, bool &notified, string &input);
    
    void server_listen_reicvmessage(_SOCKET acceptedSocket, int &connection_flag, clca::Chat &chat, mutex &m1, basic_string<_PATH_CHAR> &clientuuid, condition_variable &cv, bool &notified, string &input);

    int start_session(clca::Chat &chat, _SOCKET socket, string &input, string username, mutex &m1, int &peer_connect, basic_string<_PATH_CHAR> &peer_uuid);

    void send_new_message(_SOCKET socket,clca::Chat &chat, int file_flag, string username);

    void wait_peer(condition_variable &cv ,mutex &m1, bool &notified);

    template <typename... breadCrumbs>
    clca::msg::Message send_message(int connection_flag, clca::msg::Type type, _SOCKET socket, const char* username, breadCrumbs... crumbs) {
        clca::msg::Message ownmessage(type);
        ownmessage.setOwner(username);

        for(const char* crumb : {crumbs...}) {
            ownmessage.appendText(crumb);
        }
        
        if(connection_flag == CONNECT) //i need this check to avoid Broken Pipe Error
            ownmessage._send(socket);

        return ownmessage;
    }
}

#endif