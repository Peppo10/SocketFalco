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
#define BUFSIZE 264
#define PORT 30000
using namespace std;

SOCKET serverSocket, acceptedSocket; // socket
sockaddr_in service;                 // socket address for server socket
sockaddr_in client_addr;             // socket address client side
int client_addr_len = sizeof(sockaddr_in);
WSADATA wsadata;
message ownmessage;
bool notified;
WORD versionRequested = MAKEWORD(2, 2);
string chatbuffer = "";
string newmessages = "", newmessages_for_client = "";
int clientconnect = DISCONNECT;
int dirtyclient;
string username = "Peppo";
string clientname = "";
condition_variable cv_load_chat;
mutex m1;

int setup_server(int port);

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
            cout << "Connection established! with " << int(client_addr.sin_addr.S_un.S_un_b.s_b1) << "." << int(client_addr.sin_addr.S_un.S_un_b.s_b2) << "." << int(client_addr.sin_addr.S_un.S_un_b.s_b3) << "." << int(client_addr.sin_addr.S_un.S_un_b.s_b4) << endl;
            clientconnect = CONNECT;
        }

        char owntext[BUFSIZE] = {0};
        string input = "";
        notified = false;

        thread *awaitmessage = new thread(server_listen_reicvmessage, acceptedSocket, ref(clientconnect), ref(chatbuffer), ref(newmessages), ref(m1), ref(clientname), ref(cv_load_chat), ref(notified), ref(input));

        {
            unique_lock<mutex> ul(m1);
            cv_load_chat.wait(ul, []
                              { return (clientname == "") ? false : true; });
            char filename[21] = "/";
            strcat(filename, clientname.c_str());
            strcat(filename, ".txt");
            dirtyclient = clca::load_chat(chatbuffer, newmessages_for_client, newmessages, "/server_chat_cache", filename, username);
        }

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

        ownmessage.type = AUTH;
        strcpy(ownmessage.text, auth.c_str());
        send(acceptedSocket, (char *)&ownmessage, sizeof(ownmessage), 0);

        {
            unique_lock<mutex> ul(m1);
            cv_load_chat.wait(ul, []
                              { return notified; });

            notified = false;
        }

        ownmessage.type = NEW_MESSAGE;
        strcpy(ownmessage.text, newmessages_for_client.c_str());
        send(acceptedSocket, (char *)&ownmessage, sizeof(ownmessage), 0);

        newmessages_for_client = "";

        if (clientconnect == CONNECT_WITH_NEW_MESSAGE)
        {
            clientconnect = CONNECT;
        }
        else
        { // if is connected without new messages it prints the chatbuffer
            cout << chatbuffer + newmessages;
            cout << "You:";
        }

        do
        {
            char ch = 0;
            input = "";

            while (1)
            {
                while (!_kbhit())
                {
                }

                ch = _getch();

                if (ch == '\r')
                    break;

                if ((ch != '\b') && (input.length() < BUFSIZE - (username.size() + 3))) // 3 is the size of ":"+"\n"+"\0"
                    input += ch;

                if ((ch == '\b') && (input.size() > 0))
                    input.pop_back();

                system("cls");
                cout << chatbuffer + newmessages;
                cout << "You:" << input;
            }

            m1.lock();

            if (input.length() > 0)
            {
                if (input == "quit")
                {
                    clientconnect = DISCONNECT;
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

                if (clientconnect == CONNECT)
                {
                    ownmessage.type = MESSAGE;
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