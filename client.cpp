#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <conio.h>
#include "service.hpp"
#include "caching.hpp"

#define FAILED_TO_CONNECT 404

using namespace std;

SOCKET local_socket;
sockaddr_in server_socket_addr;
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
message ownmessage;
string chatbuffer = "";
string newmessages = "", newmessages_for_server = "";
int serverconnect = DISCONNECT, dirtyflag;
bool notified;
string username;
string servername;
condition_variable cv_print_message;
mutex m1;
int connection_flag;

int try_connection(u_long, u_short);

int main()
{

    if ((dirtyflag = clca::load_chat(chatbuffer, newmessages_for_server, newmessages, "/client_chat_cache", "/cache.txt", username)) == FILE_NOT_ALREADY_EXISTS)
    {
        do
        {
            username = "";
            system("cls");
            cout << "\033[38;2;255;255;0mFIRST ACCESS\033[0m\n";
            cout << "Enter your name(MAX 15 character):";
            cin >> username;
        } while (username.length() > 15 && username.length() > 0);
    }

    cout << "\033[38;2;255;255;0mWelcome " << username << "!\033[0m\n";

    char owntext[BUFSIZE] = {0};
    string input = "";

    if ((connection_flag = try_connection(0x7f000001, 30000)) == FAILED_TO_CONNECT)
    {
        cout << "\033[38;2;255;0;0mCANNOT CONNECT TO THE SERVER\n";
        cout << "write your message here, when you'll connect again with the server it'll receive it\033[0m\n";
        cout << "You:";
    }
    else
    {
        notified = false;
        thread *awaitmessage = new thread(client_listen_reicvmessage, local_socket, ref(serverconnect), chatbuffer, ref(newmessages), ref(m1), ref(servername), ref(cv_print_message), ref(notified), ref(input));

        // this snippet is thread safe because the server will not send anything until it receive the AUTH message from client
        ownmessage.type = AUTH;
        strcpy(ownmessage.text, username.c_str());
        strcat(ownmessage.text, "-");

        if (dirtyflag == FILE_EXIST__NEW_MESSAGE)
            strcat(ownmessage.text, "new");

        send(local_socket, (char *)&ownmessage, sizeof(ownmessage), 0);

        {
            unique_lock<mutex> ul(m1);
            cv_print_message.wait(ul, []
                                  { return notified; });

            notified = false;
        }

        ownmessage.type = NEW_MESSAGE;
        strcpy(ownmessage.text, newmessages_for_server.c_str());
        send(local_socket, (char *)&ownmessage, sizeof(ownmessage), 0);

        {
            unique_lock<mutex> ul(m1);
            cv_print_message.wait(ul, []
                                  { return notified; });

            notified = false;
        }

        if (serverconnect == CONNECT_WITH_NEW_MESSAGE)
        {
            serverconnect = CONNECT; // if is connected with new messages from server it prints the chatbuffer in service thread
        }
        else
        { // if is connected without new messages from server it prints the chatbuffer
            cout << chatbuffer + newmessages;
            cout << "You:";
        }
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
                serverconnect = DISCONNECT;
                closesocket(local_socket);
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

                return 0;
            }
            else
            {
                strcat(owntext, username.c_str());
                strcat(owntext, ":");
                strcat(owntext, input.c_str());
                strcat(owntext, "\n");
                newmessages += ("You:" + input + "\n");
            }

            if (serverconnect == CONNECT)
            {
                ownmessage.type = MESSAGE;
                strcpy(ownmessage.text, owntext);
                send(local_socket, (char *)&ownmessage, sizeof(ownmessage), 0);
            }
        }

        system("cls");
        cout << chatbuffer + newmessages;

        m1.unlock();

        memset(owntext, 0, BUFSIZE);
        cout << "You:";
    } while (1);

    WSACleanup();
}

int try_connection(u_long ip_address, u_short port)
{
    if (int result = WSAStartup(versionRequested, &wsadata))
    {
        cout << "Error during set-up: " << result << endl;
    }

    local_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (local_socket == INVALID_SOCKET)
    {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
    }

    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr.S_un.S_addr = htonl(ip_address);

    if (connect(local_socket, (sockaddr *)&server_socket_addr, sizeof(server_socket_addr)) == SOCKET_ERROR)
    {
        WSACleanup();
        return FAILED_TO_CONNECT;
    }

    serverconnect = CONNECT;

    return 1;
}