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

condition_variable cv;
mutex m1;

string username = "Server";
basic_string<_PATH_CHAR> clientuuid;
clca::Chat chat;
string newmessages_for_client;
string input;
string uuid;

int clientconnect = srv::DISCONNECT;
int file_flag;
bool notified = false;

int setup_server(int port);

void send_auth();

void prepareCUI();

int setup();

void load_data();

int main()
{
    if(setup())
        return EXIT_FAILURE;

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

            // listening thread
            new thread(srv::server_listen_reicvmessage, acceptedSocket, ref(clientconnect), ref(chat), ref(m1), ref(clientuuid), ref(cv), ref(notified), ref(input));

            // wait for client auth
            srv::wait_peer(cv, m1, notified);

            load_data();

            send_auth();

            srv::wait_peer(cv, m1, notified);

            srv::send_new_message(acceptedSocket, chat, file_flag, username);

            prepareCUI();

            srv::start_session(chat, acceptedSocket, input, username, m1, clientconnect, clientuuid);

            chat.clearQueue();
        }

        system(_CLEAR);
        cout << "listening on port " << PORT << "..." << endl;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return EXIT_SUCCESS;
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

    return EXIT_SUCCESS;
}

void send_auth()
{
    string auth;

    switch (file_flag)
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

    if (file_flag > 0)
    {
        srv::send_message(clca::msg::AUTH, acceptedSocket, uuid.c_str(), auth.c_str(), "-", to_string(file_flag).c_str());
    }
    else{
        srv::send_message(clca::msg::AUTH, acceptedSocket, uuid.c_str(),auth.c_str());
    }
}

void prepareCUI()
{
    chat.print();
    cout << "You:";
}

int setup(){
    if(clca::fileSysSetup(1))
        return EXIT_FAILURE;

    if (setup_server(PORT))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void load_data(){
    if (clca::loadUUID(1, username, uuid) == FILE_NOT_ALREADY_EXISTS)
        uuid = clca::genUUID(username);

    file_flag = clca::load_chat(chat, clientuuid);
}