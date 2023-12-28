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

#ifndef CLIENT_H
#define CLIENT_H

#include "connection.hpp"

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

    remote_socket_addr.sin_family = AF_INET;
    remote_socket_addr.sin_port = htons(port);
    remote_socket_addr.sin_addr = ip_address;

    if (connect(local_socket, (sockaddr *)&remote_socket_addr, sizeof(remote_socket_addr)) == _SOCKET_ERR)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        return FAILED_TO_CONNECT;
    }

    remote_connect = srv::CONNECT;

    return 1;
}

void send_auth()
{
    srv::send_message(remote_connect, clca::msg::AUTH, local_socket, uuid.c_str(),"");
}

void send_info(){
    file_flag = clca::load_chat(chat, remote_uuid);

    if (file_flag > 0)
    {
        srv::send_message(remote_connect, clca::msg::INFO, local_socket, username.c_str(),"new-",to_string(file_flag).c_str());
    }
    else{
        srv::send_message(remote_connect, clca::msg::INFO, local_socket, username.c_str(),"");
    }
}

void prepare_CUI()
{
    chat.consumeQueueMessages();
    chat.print();
    cout << "You:";
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

    //generate uuid and save it 
    uuid = clca::genUUID(username);
}

int setup(){
    if(clca::fileSysSetup())
        return EXIT_FAILURE;

    if (clca::loadUUID(0, username, uuid) == FILE_NOT_ALREADY_EXISTS)
        handle_new_user();

    cout << "\033[38;2;255;255;0mWelcome " << username << "!\033[0m\n";

    return EXIT_SUCCESS;
}

int start_client(int argc, char *argv[])
{

    if (argc != 1)
    {
        cout << "Only one argument <ip-address> is accepted" << endl;
        return EXIT_FAILURE;
    }
    
    if(setup())
        return EXIT_FAILURE;

    in_addr sin_addr;
    inet_pton(AF_INET, argv[0], &sin_addr);

    if (try_connection(sin_addr, 30000) == FAILED_TO_CONNECT)
    {
        cout << "\033[38;2;255;0;0mCANNOT CONNECT TO THE SERVER\033[0m\n";
        return EXIT_FAILURE;
    }
    else
    {
        // listening thread
        new thread(srv::client_listen_reicvmessage, local_socket, ref(remote_connect), ref(chat), ref(m1), ref(remote_uuid), ref(cv), ref(notified), ref(input));

        // this snippet is thread safe because the server will not send anything until it receive the AUTH message from client
        send_auth();

        srv::wait_peer(cv, m1, notified);

        send_info();

        //TCP should ensure server receives the info first ? (Header Sequence number)

        srv::send_new_message(local_socket, chat, file_flag, username);

        srv::wait_peer(cv, m1, notified);

        prepare_CUI();

        srv::start_session(chat, local_socket, input, username, m1, remote_connect, remote_uuid);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return EXIT_SUCCESS;
}

#endif