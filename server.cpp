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

#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <conio.h>
#include <condition_variable>
#include "service.hpp"
#include "caching.hpp"

#define PORT 30000

SOCKET serverSocket, acceptedSocket; // socket
sockaddr_in service;                 // socket address for server socket
sockaddr_in client_addr;             // socket address client side
int client_addr_len = sizeof(sockaddr_in);
WORD versionRequested = MAKEWORD(2, 2);
WSADATA wsadata;
srv::message ownmessage;

condition_variable cv_load_chat;
mutex m1;

string username = "Server";
string clientname = "";
string chatbuffer = "";
string newmessages = "";
string newmessages_for_client = "";
string input = "";
char owntext[BUFSIZE] = {0};

int clientconnect = srv::DISCONNECT;
int dirtyclient;
bool notified;

int setup_server(int port);

bool message_is_ready();

void send_auth();

void send_new_message();

void wait_client();

void wait_client_auth();

int main()
{
    if (setup_server(PORT))
        return -1;

    while (1)
    {
        acceptedSocket = accept(serverSocket, (sockaddr *)&client_addr, &client_addr_len);
        system("cls");

        if (acceptedSocket == INVALID_SOCKET)
        {
            cout << "Error! invalid socket!" << endl;
        }
        else
        {
            cout << "Connection established! with "
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b1)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b2)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b3)
                 << "."
                 << int(client_addr.sin_addr.S_un.S_un_b.s_b4) << endl;

            clientconnect = srv::CONNECT;
        }

        notified = false;

        new thread(srv::server_listen_reicvmessage, acceptedSocket, ref(clientconnect), ref(chatbuffer), ref(newmessages), ref(m1), ref(clientname), ref(cv_load_chat), ref(notified), ref(input));

        wait_client_auth();

        send_auth();

        wait_client();

        send_new_message();

        if (clientconnect == srv::CONNECT_WITH_NEW_MESSAGE)
        {
            clientconnect = srv::CONNECT;
        }
        else
        { // if is connected without new messages it prints the chatbuffer
            cout << chatbuffer + newmessages;
            cout << "You:";
        }

        do
        {
            input = "";

            while (!message_is_ready())
            {
            }

            m1.lock();

            if (input.length() > 0)
            {
                if (input == "quit")
                {
                    clientconnect = srv::DISCONNECT;
                    closesocket(acceptedSocket);
                    m1.unlock();

                    string full_chat = chatbuffer + newmessages;
                    clca::save_chat(full_chat, username);
                    clientname = "";
                    chatbuffer = "";
                    newmessages = "";
                    break;
                }
                else
                {
                    strcat(owntext, username.c_str());
                    strcat(owntext, ":");
                    strcat(owntext, input.c_str());
                    strcat(owntext, "\n");
                    newmessages += ("You:" + input + "\n");
                }

                if (clientconnect == srv::CONNECT)
                {
                    ownmessage.type = srv::MESSAGE;
                    strcpy(ownmessage.text, owntext);
                    send(acceptedSocket, (char *)&ownmessage, sizeof(ownmessage), 0);
                }
            }

            system("cls");
            cout << chatbuffer + newmessages;

            m1.unlock();

            memset(owntext, 0, BUFSIZE);
            cout << "You:";
        } while (1);

        system("cls");
        cout << "listening on port " << PORT << "..." << endl;
    }
    WSACleanup();
    return 0;
}

int setup_server(int port)
{
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

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        return -2;
    }
    else
    {
        cout << "socket() is OK!" << endl;
    }

    service.sin_addr.s_addr = htonl(INADDR_ANY); // listen for all interfaces
    service.sin_port = htons(port);              // set the port
    service.sin_family = AF_INET;                // set the family

    if (bind(serverSocket, (sockaddr *)&service, sizeof(service)) == SOCKET_ERROR)
    {
        cout << "bind() failed! " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -3;
    }
    else
    {
        cout << "bind() is OK!" << endl;
    }

    if (listen(serverSocket, 3) == SOCKET_ERROR)
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

bool message_is_ready()
{
    char ch = 0;

    while (!_kbhit())
    {
    }

    ch = _getch();

    if (ch == '\r')
        return true;

    if ((ch != '\b') && (input.length() < BUFSIZE - (username.size() + 3))) // 3 is the size of ":"+"\n"+"\0"
        input += ch;

    if ((ch == '\b') && (input.size() > 0))
        input.pop_back();

    system("cls");
    cout << chatbuffer + newmessages;
    cout << "You:" << input;

    return false;
}

void send_auth()
{
    string auth;

    switch (dirtyclient)
    {
    case FILE_ALREADY_EXISTS:
        auth = "\033[38;2;0;255;0mUser authenticated, loading the chat...\033[0m\n";
        break;

    case FILE_EXIST__NEW_MESSAGE:
        auth = "\033[38;2;0;255;0mUser authenticated, loading the chat with \033[4mnew messages!\033[0m\n";
        break;

    case FILE_NOT_ALREADY_EXISTS:
        auth = "\033[38;2;100;100;255mNew user,initialization...\033[0m\n";
        break;
    }

    ownmessage.type = srv::AUTH;
    strcpy(ownmessage.text, auth.c_str());
    send(acceptedSocket, (char *)&ownmessage, sizeof(ownmessage), 0);
}

void send_new_message()
{
    ownmessage.type = srv::NEW_MESSAGE;
    strcpy(ownmessage.text, newmessages_for_client.c_str());
    send(acceptedSocket, (char *)&ownmessage, sizeof(ownmessage), 0);

    newmessages_for_client = "";
}

void wait_client()
{
    unique_lock<mutex> ul(m1);
    cv_load_chat.wait(ul, []
                      { return notified; });

    notified = false;
}

void wait_client_auth()
{

    unique_lock<mutex> ul(m1);
    cv_load_chat.wait(ul, []
                      { return (clientname == "") ? false : true; });
    char filename[21] = "/";
    strcat(filename, clientname.c_str());
    strcat(filename, ".txt");
    dirtyclient = clca::load_chat(chatbuffer, newmessages_for_client, newmessages, "/server_chat_cache", filename, username);
}