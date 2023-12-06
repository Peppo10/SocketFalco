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

#include "./service/service.hpp"
#include "./caching/caching.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
// #pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib

#define _CLOSE_SOCKET closesocket
#define _SOCKET_INV INVALID_SOCKET
#define _SOCKET_ERR SOCKET_ERROR
#define _CLEAR "cls"
#elif __linux__
#include <arpa/inet.h>
#include <sys/types.h>

#define _CLOSE_SOCKET close
#define _SOCKET_INV -1
#define _SOCKET_ERR -1
#define _CLEAR "clear"
#endif

#include <thread>
#define FAILED_TO_CONNECT 404

_SOCKET local_socket;
sockaddr_in server_socket_addr;

#ifdef _WIN32
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
#endif

condition_variable cv_print_message;
mutex m1;

string chatbuffer = "";
string newmessages = "";
string newmessages_for_server = "";
string input = "";
string username;
string servername;

int connection_flag;
char owntext[BUFSIZE] = {0};
int serverconnect = srv::DISCONNECT, dirtyflag;
bool notified;

int try_connection(in_addr, u_short);

void send_auth();

void send_new_message();

void wait_server();

void handle_new_user();

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        cout << "Only one argument <ip-address> is accepted" << endl;
        return EXIT_FAILURE;
    }

    in_addr sin_addr;

    inet_pton(AF_INET, argv[1], &sin_addr);

    if ((dirtyflag = clca::load_chat(chatbuffer, newmessages_for_server, newmessages, "/client_chat_cache", "/cache.txt", username)) == FILE_NOT_ALREADY_EXISTS)
    {
        handle_new_user();
    }

    cout << "\033[38;2;255;255;0mWelcome " << username << "!\033[0m\n";

    if ((connection_flag = try_connection(sin_addr, 30000)) == FAILED_TO_CONNECT)
    {
        cout << "\033[38;2;255;0;0mCANNOT CONNECT TO THE SERVER\n";
        cout << "write your message here, when you'll connect again with the server will receive it\033[0m\n";
        cout << "You:";
    }
    else
    {
        notified = false;

        // listening thread
        new thread(srv::client_listen_reicvmessage, local_socket, ref(serverconnect), chatbuffer, ref(newmessages), ref(m1), ref(servername), ref(cv_print_message), ref(notified), ref(input));

        // this snippet is thread safe because the server will not send anything until it receive the AUTH message from client
        send_auth();

        wait_server();

        send_new_message();

        wait_server();

        if (serverconnect == srv::CONNECT_WITH_NEW_MESSAGE)
        {
            serverconnect = srv::CONNECT; // if is connected with new messages from server it prints the chatbuffer in service thread
        }
        else
        { // if is connected without new messages from server it prints the chatbuffer
            cout << chatbuffer + newmessages;
            cout << "You:";
        }
    }

    do
    {
        input = "";
        cout << "\033[s";

        cin.clear();

        while (!msg::message_is_ready(input, username))
        {
        }

        m1.lock();

        if (input.length() > 0)
        {
            if (input == "quit")
            {
                serverconnect = srv::DISCONNECT;
                _CLOSE_SOCKET(local_socket);
                m1.unlock();

                string full_chat;
                if (connection_flag == FAILED_TO_CONNECT)
                {
                    full_chat = chatbuffer + "\033[38;2;255;0;0mThe server is disconnected\033[0m\n" + newmessages;
                    clca::save_chat(full_chat, username);
                }
                else
                {
                    full_chat = chatbuffer + newmessages;
                    clca::save_chat(full_chat, username);
                }

                return EXIT_SUCCESS;
            }
            else
            {
                strcat(owntext, username.c_str());
                strcat(owntext, ":");
                strcat(owntext, input.c_str());
                strcat(owntext, "\n");
                newmessages += ("You:" + input + "\n");
            }

            if (serverconnect == srv::CONNECT)
            {
                msg::Message ownmessage(msg::Message::Type::MESSAGE);
                ownmessage.appendText(owntext);
                ownmessage._send(local_socket);
            }
        }

        m1.unlock();

        memset(owntext, 0, BUFSIZE);
        cout << "\nYou:";
    } while (1);

#ifdef _WIN32
    WSACleanup();
#endif
}

int try_connection(in_addr ip_address, u_short port)
{
#ifdef _WIN32
    if (int result = WSAStartup(versionRequested, &wsadata))
    {
        cout << "Error during set-up: " << result << endl;
    }
#endif

    local_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (local_socket == _SOCKET_INV)
    {
#ifdef _WIN32
        cout << "Error at socket(): " << WSAGetLastError() << endl;
#elif __linux__
        cout << "Error at socket(), exit code: " << local_socket << endl;
#endif
    }

    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr = ip_address;

    if (connect(local_socket, (sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) == _SOCKET_ERR)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        return FAILED_TO_CONNECT;
    }

    serverconnect = srv::CONNECT;

    return 1;
}

void send_auth()
{
    msg::Message ownmessage(msg::Message::Type::AUTH);
    ownmessage.appendText(owntext);
    ownmessage.appendText(username.c_str());
    ownmessage.appendText("-");

    if (dirtyflag == FILE_EXIST__NEW_MESSAGE)
        ownmessage.appendText("new");

    ownmessage._send(local_socket);
}

void send_new_message()
{
    msg::Message ownmessage(msg::Message::Type::NEW_MESSAGE);
    ownmessage.appendText(newmessages_for_server.c_str());
    ownmessage._send(local_socket);
}

void wait_server()
{
    unique_lock<mutex> ul(m1);
    cv_print_message.wait(ul, []
                          { return notified; });

    notified = false;
}

void handle_new_user()
{
    do
    {
        username = "";
        system(_CLEAR);
        cout << "\033[38;2;255;255;0mFIRST ACCESS\033[0m\n";
        cout << "Enter your name(MAX 15 character):";
        getline(cin, username);
    } while (username.length() > 15 && username.length() > 0);
}