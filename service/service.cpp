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

#include "service.hpp"
#include <winsock2.h>
#include <iostream>
#include <condition_variable>
#include <mutex>

using namespace srv;

char CLIENT_DISCONNECT[] = "\033[38;2;255;0;0mThe client is disconnected\033[0m\n";
char SERVER_DISCONNECT[] = "\033[38;2;255;0;0mThe server is disconnected\033[0m\n";
char NEW_MESSAGES[] = "\033[38;2;255;255;0mNew messages!\033[0m\n";

void srv::client_listen_reicvmessage(SOCKET local_socket, int &connection_flag, const string chatbuffer, string &my_new_messages, mutex &m1, string &servername, condition_variable &cv, bool &notified, string &input)
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
                handle_message(servermessage.text, chatbuffer, my_new_messages, m1, input);
                break;
            case NEW_MESSAGE:
                handle_new_messages(servermessage.text, notified, chatbuffer, my_new_messages, cv, m1);
                break;
            }
        }
        else
        {
            handle_disconnect_message(chatbuffer, my_new_messages, m1, input, connection_flag, SERVER_DISCONNECT);
            break;
        }

        memset(servermessage.text, 0, BUFSIZE);
    }
}

void srv::server_listen_reicvmessage(SOCKET acceptedSocket, int &connection_flag, string &chatbuffer, string &my_new_messages, mutex &m1, string &clientname, condition_variable &cv, bool &notified, string &input)
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
                handle_message(clientmessage.text, chatbuffer, my_new_messages, m1, input);
                break;
            case NEW_MESSAGE:
                handle_new_messages(clientmessage.text, notified, chatbuffer, my_new_messages, cv, m1);
                break;
            }
        }
        else
        {
            handle_disconnect_message(chatbuffer, my_new_messages, m1, input, connection_flag, CLIENT_DISCONNECT);
            break;
        }
        memset(clientmessage.text, 0, BUFSIZE);
    }
}

void srv::handle_new_messages(char newmessages[BUFSIZE], bool &notified, string chatbuffer, string &my_new_messages, condition_variable &cv, mutex &m1)
{
    if (strcmp(newmessages, "") == 0)
    {
        notified = true;

        cv.notify_one();
        m1.unlock();

        return;
    }

    cout << chatbuffer + my_new_messages;
    my_new_messages += newmessages;
    cout << NEW_MESSAGES;
    cout << newmessages;
    cout << "You:";

    notified = true;

    cv.notify_one();
    m1.unlock();
}

void srv::handle_message(char newmessages[BUFSIZE], string chatbuffer, string &my_new_messages, mutex &m1, string input)
{
    my_new_messages += newmessages;
    system("cls");
    cout << chatbuffer + my_new_messages;
    cout << "You:" << input;
    m1.unlock();
}

void srv::handle_disconnect_message(string chatbuffer, string &my_new_messages, mutex &m1, string input, int &connection_flag, char message[])
{
    system("cls");
    my_new_messages += message;
    cout << chatbuffer + my_new_messages;
    connection_flag = DISCONNECT;
    cout << "You:" << input;
    m1.unlock();
}