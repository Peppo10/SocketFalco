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

SOCKET local_socket;
sockaddr_in server_socket_addr;
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
srv::message ownmessage;

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

int try_connection(u_long, u_short);

bool message_is_ready();

void send_auth();

void send_new_message();

void wait_server();

void handle_new_user();

int main()
{

    if ((dirtyflag = clca::load_chat(chatbuffer, newmessages_for_server, newmessages, "/client_chat_cache", "/cache.txt", username)) == FILE_NOT_ALREADY_EXISTS)
    {
        handle_new_user();
    }

    cout << "\033[38;2;255;255;0mWelcome " << username << "!\033[0m\n";

    if ((connection_flag = try_connection(0x7f000001, 30000)) == FAILED_TO_CONNECT)
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

        while (!message_is_ready())
        {
        }

        m1.lock();

        if (input.length() > 0)
        {
            if (input == "quit")
            {
                serverconnect = srv::DISCONNECT;
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

            if (serverconnect == srv::CONNECT)
            {
                ownmessage.type = srv::MESSAGE;
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

    serverconnect = srv::CONNECT;

    return 1;
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
    ownmessage.type = srv::AUTH;
    strcpy(ownmessage.text, username.c_str());
    strcat(ownmessage.text, "-");

    if (dirtyflag == FILE_EXIST__NEW_MESSAGE)
        strcat(ownmessage.text, "new");

    send(local_socket, (char *)&ownmessage, sizeof(ownmessage), 0);
}

void send_new_message()
{
    ownmessage.type = srv::NEW_MESSAGE;
    strcpy(ownmessage.text, newmessages_for_server.c_str());
    send(local_socket, (char *)&ownmessage, sizeof(ownmessage), 0);
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
        system("cls");
        cout << "\033[38;2;255;255;0mFIRST ACCESS\033[0m\n";
        cout << "Enter your name(MAX 15 character):";
        cin >> username;
    } while (username.length() > 15 && username.length() > 0);
}