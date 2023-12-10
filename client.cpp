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
#ifdef _WIN32
#include <ws2tcpip.h>

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
#include <iostream>
#include <string.h>
#include <condition_variable>
#include <mutex>
#include "./service/service.hpp"
#define FAILED_TO_CONNECT 404

using namespace std;

_SOCKET local_socket;
sockaddr_in server_socket_addr;

#ifdef _WIN32
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
#endif

condition_variable cv_print_message;
mutex m1;

clca::Chat chat;
string newmessages = "";
string newmessages_for_server = "";
string input = "";
string username;
string servername;

int connection_flag;
int serverconnect = srv::DISCONNECT, fileflag;
bool notified;

int try_connection(in_addr, u_short);

void send_auth();

void send_new_message();

void wait_server();

void handle_new_user();

void prepareCUI();

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        cout << "Only one argument <ip-address> is accepted" << endl;
        return EXIT_FAILURE;
    }

    in_addr sin_addr;

    inet_pton(AF_INET, argv[1], &sin_addr);

    if ((fileflag = clca::load_chat(chat, "/client_chat_cache", "/cache.txt", username)) == FILE_NOT_ALREADY_EXISTS)
    {
        handle_new_user();
    }

    cout << "\033[38;2;255;255;0mWelcome " << username << "!\033[0m\n";

    if ((connection_flag = try_connection(sin_addr, 30000)) == FAILED_TO_CONNECT)
    {
        cout << "\033[38;2;255;0;0mCANNOT CONNECT TO THE SERVER\n";
        cout << "write your message here, on the next session the server will receive it\033[0m\n";
        cout << "You:";
    }
    else
    {
        notified = false;

        // listening thread
        new thread(srv::client_listen_reicvmessage, local_socket, ref(serverconnect), ref(chat), ref(m1), ref(servername), ref(cv_print_message), ref(notified), ref(input));

        // this snippet is thread safe because the server will not send anything until it receive the AUTH message from client
        send_auth();

        wait_server();

        send_new_message();

        wait_server();

        prepareCUI();
    }

    do
    {
        input = "";
        cout << "\033[s";

        cin.clear();

        while (!clca::msg::message_is_ready(input, username))
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

                clca::save_chat(chat, username);

                return EXIT_SUCCESS;
            }
            else
            {
                clca::msg::Message ownmessage((connection_flag == FAILED_TO_CONNECT) || (serverconnect != srv::CONNECT) ? clca::msg::Message::Type::NEW_MESSAGE : clca::msg::Message::Type::MESSAGE);
                ownmessage.setOwner(username.c_str());
                ownmessage.appendText(input.c_str());
                chat.addMessage(ownmessage);
                cout << "\033[G\033[J";
                ownmessage.print();

                if (serverconnect == srv::CONNECT)
                {
                    ownmessage._send(local_socket);
                }
            }
        }
        else
        {
            cout << "\033[G\033[K";
        }

        m1.unlock();
        cout << "You:";
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
    clca::msg::Message ownmessage(clca::msg::Message::Type::AUTH);
    ownmessage.setOwner(username.c_str());

    if (fileflag > 0)
    {
        ownmessage.appendText("new-");
        ownmessage.appendText(to_string(fileflag).c_str());
    }

    ownmessage._send(local_socket);
}

void send_new_message()
{
    size_t chatSize = chat.getSize();
    if (fileflag > 0)
    {
        for (size_t i = chatSize - fileflag; i < chatSize; i++)
        {
            chat.getAt(i)._send(local_socket);
            chat.getAt(i).setType(clca::msg::Message::Type::MESSAGE);
        }
    }
    else
    {
        clca::msg::Message ownmessage(clca::msg::Message::Type::NEW_MESSAGE);
        ownmessage.setOwner(username.c_str());
        ownmessage.appendText("\0");
        ownmessage._send(local_socket);
    }
}

void prepareCUI()
{
    chat.consumeQueueMessages();
    chat.print();
    cout << "You:";
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