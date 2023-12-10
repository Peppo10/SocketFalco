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
#include <conio.h>

#define _CLOSE_SOCKET closesocket
#define _SOCKET_INV INVALID_SOCKET
#define _SOCKET_ERR SOCKET_ERROR
#define _CLEAR "cls"
typedef int _ADDR_LEN;
#elif __linux__
#include <arpa/inet.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#define _CLOSE_SOCKET close
#define _SOCKET_INV -1
#define _SOCKET_ERR -1
#define _CLEAR "clear"
typedef u_int _ADDR_LEN;
#endif

#include <thread>
#define PORT 30000

_SOCKET serverSocket, acceptedSocket; // socket
sockaddr_in service;                  // socket address for server socket
sockaddr_in client_addr;              // socket address client side
_ADDR_LEN client_addr_len = sizeof(sockaddr_in);

#ifdef _WIN32
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
#endif

condition_variable cv_load_chat;
mutex m1;

string username = "Server";
string clientname = "";
clca::Chat chat;
string newmessages = "";
string newmessages_for_client = "";
string input = "";

int clientconnect = srv::DISCONNECT;
int dirtyclient;
bool notified;

int setup_server(int port);

void send_auth();

void send_new_message();

void wait_client();

void wait_client_auth();

void prepareCUI();

int main()
{
    if (setup_server(PORT))
        return -1;

    while (1)
    {
        acceptedSocket = accept(serverSocket, (sockaddr *)&client_addr, &client_addr_len);
        system(_CLEAR);

        if (acceptedSocket == _SOCKET_INV)
        {
            cout << "Error! invalid socket!" << endl;
        }
        else
        {

#ifdef _WIN32
            cout << "Connection established! with "
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b1)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b2)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b3)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b4) << endl;
#elif __linux__
            cout << "Connection established! with "
                 << int(client_addr.sin_addr.s_addr) << endl;
#endif

            clientconnect = srv::CONNECT;
        }

        notified = false;

        new thread(srv::server_listen_reicvmessage, acceptedSocket, ref(clientconnect), ref(chat), ref(m1), ref(clientname), ref(cv_load_chat), ref(notified), ref(input));

        wait_client_auth();

        send_auth();

        wait_client();

        send_new_message();

        prepareCUI();

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
                    clientconnect = srv::DISCONNECT;
                    _CLOSE_SOCKET(acceptedSocket);
                    m1.unlock();

                    clca::save_chat(chat, username);
                    clientname = "";
                    chat.clear();
                    newmessages = "";
                    break;
                }
                else
                {
                    clca::msg::Message ownmessage((clientconnect != srv::CONNECT) ? clca::msg::Message::Type::NEW_MESSAGE : clca::msg::Message::Type::MESSAGE);
                    ownmessage.setOwner(username.c_str());
                    ownmessage.appendText(input.c_str());
                    chat.addMessage(ownmessage);
                    cout << "\033[G\033[J";
                    ownmessage.print();

                    if (clientconnect == srv::CONNECT)
                    {
                        ownmessage._send(acceptedSocket);
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

        chat.clearQueue();
        system(_CLEAR);
        cout << "listening on port " << PORT << "..." << endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

int setup_server(int port)
{
#ifdef _WIN32
    if (WSAStartup(versionRequested, &wsadata))
    {
        cout << "dll file not found" << endl;
        return -1;
    }
    else
    {
        cout << "dll file found!" << endl;
        cout << wsadata.szSystemStatus << endl;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == _SOCKET_INV)
    {
#ifdef _WIN32
        cout << "Error at socket(): " << WSAGetLastError() << endl;
#elif __linux__
        cout << "Error at socket(), exit code: " << _SOCKET_INV << endl;
#endif

        return -2;
    }
    else
    {
        cout << "socket() is OK!" << endl;
    }

    service.sin_addr.s_addr = htonl(INADDR_ANY); // listen for all interfaces
    service.sin_port = htons(port);              // set the port
    service.sin_family = AF_INET;                // set the family

    if (bind(serverSocket, (sockaddr *)&service, sizeof(service)) == _SOCKET_ERR)
    {
#ifdef _WIN32
        cout << "bind() failed! " << WSAGetLastError() << endl;
#elif __linux__
        cout << "bind() failed!, exit code: " << _SOCKET_ERR << endl;
#endif

        _CLOSE_SOCKET(serverSocket);

#ifdef _WIN32
        WSACleanup();
#endif

        return -3;
    }
    else
    {
        cout << "bind() is OK!" << endl;
    }

    if (listen(serverSocket, 3) == _SOCKET_ERR)
    {
        cout << "listen(): Error listen on socket!" << endl;
        return -4;
    }
    else
    {
        cout << "listening on port " << port << "..." << endl;
    }

    return 0;
}

void send_auth()
{
    string auth;

    switch (dirtyclient)
    {
    case FILE_ALREADY_EXISTS:
        auth = "\033[38;2;0;255;0mUser authenticated, loading the chat...\033[0m";
        break;

    case FILE_NOT_ALREADY_EXISTS:
        auth = "\033[38;2;100;100;255mNew user,initialization...\033[0m";
        break;
    default:
        auth = "\033[38;2;0;255;0mUser authenticated, loading the chat with \033[4mnew messages!\033[0m";
        break;
    }

    clca::msg::Message ownmessage(clca::msg::Message::Type::AUTH);
    ownmessage.setOwner(username.c_str());
    ownmessage.appendText(auth.c_str());

    if (dirtyclient > 0)
    {
        ownmessage.appendText("-");
        ownmessage.appendText(to_string(dirtyclient).c_str());
    }

    ownmessage._send(acceptedSocket);
}

void send_new_message()
{
    size_t chatSize = chat.getSize();
    if (dirtyclient > 0)
    {
        for (size_t i = chatSize - dirtyclient; i < chatSize; i++)
        {
            chat.getAt(i)._send(acceptedSocket);
            chat.getAt(i).setType(clca::msg::Message::Type::MESSAGE);
        }
    }
    else
    {
        clca::msg::Message ownmessage(clca::msg::Message::Type::NEW_MESSAGE);
        ownmessage.setOwner(username.c_str());
        ownmessage.appendText("\0");
        ownmessage._send(acceptedSocket);
    }

    chat.consumeQueueMessages();
}

void wait_client()
{
    unique_lock<mutex> ul(m1);
    cv_load_chat.wait(ul, []
                      { return notified; });

    notified = false;
}

void prepareCUI()
{
    chat.print();
    cout << "You:";
}

void wait_client_auth()
{

    unique_lock<mutex> ul(m1);
    cv_load_chat.wait(ul, []
                      { return (clientname == "") ? false : true; });
    char filename[21] = "/";
    strcat(filename, clientname.c_str());
    strcat(filename, ".txt");
    dirtyclient = clca::load_chat(chat, "/server_chat_cache", filename, username);
}