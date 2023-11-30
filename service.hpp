#ifndef SERVICE_H
#define SERVICE_H
#define BUFSIZE 264
#include <winsock2.h>
#include <iostream>
#include <condition_variable>
#include <mutex>

using namespace std;

enum typelist
{
    AUTH,
    MESSAGE,
    NEW_MESSAGE
};

enum connection_type_lis
{
    CONNECT_WITH_NEW_MESSAGE,
    CONNECT,
    DISCONNECT
};

struct message
{
    u_short type;
    char text[BUFSIZE];
};

void client_listen_reicvmessage(SOCKET local_socket, int &connection_flag, const string chatbuffer, string &my_new_messages, mutex &m1, string &servername, condition_variable &cv, bool &notified, string &input)
{
    message servermessage;
    while (1)
    {
        int result = recv(local_socket, (char *)&servermessage, sizeof(servermessage), 0);

        m1.lock();

        if (connection_flag == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the server quit
            m1.unlock();
            break;
        }

        if (result > 0)
        {
            switch (servermessage.type)
            {
            case AUTH:
                cout << servermessage.text;

                if (strcmp(servermessage.text, "\033[38;2;0;255;0mUser authenticated, loading the chat with \033[4mnew messages!\033[0m\n") == 0)
                    connection_flag = CONNECT_WITH_NEW_MESSAGE;

                strcpy(servermessage.text, "");
                notified = true;
                cv.notify_one();
                m1.unlock();
                break;
            case MESSAGE:
                my_new_messages += servermessage.text;
                system("cls");
                cout << chatbuffer + my_new_messages;
                cout << "You:" << input;
                m1.unlock();
                break;
            case NEW_MESSAGE:

                if (strcmp(servermessage.text, "") == 0)
                {
                    notified = true;

                    cv.notify_one();
                    m1.unlock();

                    break;
                }

                cout << chatbuffer + my_new_messages;
                my_new_messages += servermessage.text;
                cout << "\033[38;2;255;255;0mNew messages!\033[0m\n";
                cout << servermessage.text;
                cout << "You:";

                notified = true;
                cv.notify_one();
                m1.unlock();
                break;
            }
        }
        else
        {
            system("cls");
            my_new_messages += "\033[38;2;255;0;0mThe server is disconnected\033[0m\n";
            cout << chatbuffer + my_new_messages;
            connection_flag = DISCONNECT;
            cout << "You:";
            m1.unlock();
            break;
        }

        memset(servermessage.text, 0, BUFSIZE);
    }
}

void server_listen_reicvmessage(SOCKET acceptedSocket, int &connection_flag, string &chatbuffer, string &my_new_messages, mutex &m1, string &clientname, condition_variable &cv, bool &notified, string &input)
{
    message clientmessage;
    while (1)
    {
        int result = recv(acceptedSocket, (char *)&clientmessage, sizeof(clientmessage), 0);

        m1.lock();

        if (connection_flag == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the client quit
            m1.unlock();
            break;
        }

        if (result > 0)
        {
            switch (clientmessage.type)
            {
            case AUTH:
                clientname = strtok(clientmessage.text, "-");
                if (strtok(NULL, "-") != nullptr)
                {
                    connection_flag = CONNECT_WITH_NEW_MESSAGE;
                }

                strcpy(clientmessage.text, "");

                cv.notify_one();
                m1.unlock();
                break;
            case MESSAGE:
                my_new_messages += clientmessage.text;
                system("cls");
                cout << chatbuffer + my_new_messages;
                cout << "You:" << input;
                m1.unlock();
                break;
            case NEW_MESSAGE:

                if (strcmp(clientmessage.text, "") == 0)
                {
                    notified = true;

                    cv.notify_one();
                    m1.unlock();

                    break;
                }

                cout << chatbuffer + my_new_messages;
                my_new_messages += clientmessage.text;
                cout << "\033[38;2;255;255;0mNew messages!\033[0m\n";
                cout << clientmessage.text;
                cout << "You:";

                notified = true;

                cv.notify_one();
                m1.unlock();
                break;
            }
        }
        else
        {
            system("cls");
            my_new_messages += "\033[38;2;255;0;0mThe client is disconnected\033[0m\n";
            cout << chatbuffer + my_new_messages;
            connection_flag = DISCONNECT;
            cout << "You:";
            m1.unlock();
            break;
        }
        memset(clientmessage.text, 0, BUFSIZE);
    }
}

#endif