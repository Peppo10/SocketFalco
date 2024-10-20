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

#ifndef SERVER_H
#define SERVER_H

#include "connection.hpp"
#include "service.hpp"

Session* serverSession;

int setup_server(int port)
{
#ifdef _WIN32
    if (WSAStartup(serverSession->versionRequested, &(serverSession->wsadata)))
    {
        cout << "dll file not found" << endl;
        return -1;
    }
#endif

    serverSession->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSession->listen_socket == _SOCKET_INV)
    {
#ifdef _WIN32
        cout << "Error at socket(): " << WSAGetLastError() << endl;
#elif __linux__
        cout << "Error at socket(), exit code: " << _SOCKET_INV << endl;
#endif

        return -2;
    }

    serverSession->local_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen for all interfaces
    serverSession->local_socket_addr.sin_port = htons(port);              // set the port
    serverSession->local_socket_addr.sin_family = AF_INET;                // set the family

    if (bind(serverSession->listen_socket, (sockaddr *)&(serverSession->local_socket_addr), sizeof(serverSession->local_socket_addr)) == _SOCKET_ERR)
    {
#ifdef _WIN32
        cout << "bind() failed! " << WSAGetLastError() << endl;
#elif __linux__
        cout << "bind() failed!, exit code: " << _SOCKET_ERR << endl;
#endif

        _CLOSE_SOCKET(serverSession->listen_socket);

#ifdef _WIN32
        WSACleanup();
#endif

        return -3;
    }

    if (listen(serverSession->listen_socket, 3) == _SOCKET_ERR)
    {
        cout << "listen(): Error listen on socket!" << endl;
        return -4;
    }
    else
    {
        cout << "\033[38;2;100;100;255mServer setup succesfully!\033[0m";
        cout << "\nlistening on port " << port << "..." << endl;
    }

    return EXIT_SUCCESS;
}

void send_auth_()
{
    srv::send_message(clca::msg::AUTH, serverSession->username.c_str(), serverSession->uuid.c_str());
}

void send_info_(){
    string auth;

    switch (serverSession->file_flag)
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

    if (serverSession->file_flag > 0)
    {
        srv::send_message(clca::msg::INFO, serverSession->username.c_str(), auth.c_str(), "-", to_string(serverSession->file_flag).c_str());
    }
    else{
        srv::send_message(clca::msg::INFO, serverSession->username.c_str(), auth.c_str());
    }
}

void prepareCUI()
{
    serverSession->chat.print(true);
    cout << "You:";
}

int setup_(){
    if(clca::fileSysSetup())
        return EXIT_FAILURE;

    if (setup_server(PORT))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void load_data(){
    if (clca::loadUUID(1, serverSession->username, serverSession->uuid) == FILE_NOT_ALREADY_EXISTS)
        serverSession->uuid = clca::genUUID(serverSession->username);

    if(serverSession->temporary)
        serverSession->uuid = "";

    serverSession->file_flag = clca::load_chat(serverSession->chat, serverSession->remote_uuid);
}

int start_server(bool temporarySession = false)
{
    serverSession = Session::getInstance();
    serverSession->temporary = temporarySession;
    
    if(setup_())
        return EXIT_FAILURE;

    while (1)
    {
        serverSession->remote_socket = accept(serverSession->listen_socket, (sockaddr *)&(serverSession->remote_socket_addr), &(serverSession->client_addr_len));

        system(_CLEAR);

        if (serverSession->remote_socket == _SOCKET_INV)
        {
            cout << "Error! invalid socket!" << endl;
        }
        else
        {

#ifdef _WIN32
            cout << "Connection established! with "
                 << int(serverSession->remote_socket_addr.sin_addr.S_un.S_un_b.s_b1)
                 << "."
                 << int(serverSession->remote_socket_addr.sin_addr.S_un.S_un_b.s_b2)
                 << "."
                 << int(serverSession->remote_socket_addr.sin_addr.S_un.S_un_b.s_b3)
                 << "."
                 << int(serverSession->remote_socket_addr.sin_addr.S_un.S_un_b.s_b4) << endl;
#elif __linux__
            cout << "Connection established! with "
                 << int(serverSession->remote_socket_addr.sin_addr.s_addr) << endl;
#endif

            serverSession->remote_connect = CONNECT;

            // listening thread
            new thread(srv::server_listen_reicvmessage);

            // wait for client auth
            srv::wait_peer();

            load_data();

            send_auth_();

            send_info_();

            srv::wait_peer();

            srv::send_new_message();

            prepareCUI();

            srv::start_session();
        }

        system(_CLEAR);
        cout << "listening on port " << PORT << "..." << endl;
        Session::clearInstance();
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return EXIT_SUCCESS;
}

#endif