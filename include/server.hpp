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

    local_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (local_socket == _SOCKET_INV)
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

    if (bind(local_socket, (sockaddr *)&service, sizeof(service)) == _SOCKET_ERR)
    {
#ifdef _WIN32
        cout << "bind() failed! " << WSAGetLastError() << endl;
#elif __linux__
        cout << "bind() failed!, exit code: " << _SOCKET_ERR << endl;
#endif

        _CLOSE_SOCKET(local_socket);

#ifdef _WIN32
        WSACleanup();
#endif

        return -3;
    }
    else
    {
        cout << "bind() is OK!" << endl;
    }

    if (listen(local_socket, 3) == _SOCKET_ERR)
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

void send_auth_s()
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
        srv::send_message(remote_connect, clca::msg::AUTH, acceptedSocket, uuid.c_str(), auth.c_str(), "-", to_string(file_flag).c_str());
    }
    else{
        srv::send_message(remote_connect ,clca::msg::AUTH, acceptedSocket, uuid.c_str(),auth.c_str());
    }
}

void prepareCUI()
{
    chat.print();
    cout << "You:";
}

int setup_s(){
    if(clca::fileSysSetup())
        return EXIT_FAILURE;

    if (setup_server(PORT))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void load_data(){
    if (clca::loadUUID(1, username, uuid) == FILE_NOT_ALREADY_EXISTS)
        uuid = clca::genUUID(username);

    file_flag = clca::load_chat(chat, remote_uuid);
}

int start_server()
{
    if(setup_s())
        return EXIT_FAILURE;

    while (1)
    {
        acceptedSocket = accept(local_socket, (sockaddr *)&remote_socket_addr, &client_addr_len);
        system(_CLEAR);

        if (acceptedSocket == _SOCKET_INV)
        {
            cout << "Error! invalid socket!" << endl;
        }
        else
        {

#ifdef _WIN32
            cout << "Connection established! with "
                 << int(remote_socket_addr.sin_addr.S_un.S_un_b.s_b1)
                 << "."
                 << int(remote_socket_addr.sin_addr.S_un.S_un_b.s_b2)
                 << "."
                 << int(remote_socket_addr.sin_addr.S_un.S_un_b.s_b3)
                 << "."
                 << int(remote_socket_addr.sin_addr.S_un.S_un_b.s_b4) << endl;
#elif __linux__
            cout << "Connection established! with "
                 << int(remote_socket_addr.sin_addr.s_addr) << endl;
#endif

            remote_connect = srv::CONNECT;

            // listening thread
            new thread(srv::server_listen_reicvmessage, acceptedSocket, ref(remote_connect), ref(chat), ref(m1), ref(remote_uuid), ref(cv), ref(notified), ref(input));

            // wait for client auth
            srv::wait_peer(cv, m1, notified);

            load_data();

            send_auth_s();

            srv::wait_peer(cv, m1, notified);

            srv::send_new_message(acceptedSocket, chat, file_flag, username);

            prepareCUI();

            srv::start_session(chat, acceptedSocket, input, username, m1, remote_connect, remote_uuid);

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

#endif