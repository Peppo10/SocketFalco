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

#include "../include/service.hpp"

char CLIENT_DISCONNECT[] = "\033[38;2;255;0;0mThe client is disconnected\033[0m\n";
char SERVER_DISCONNECT[] = "\033[38;2;255;0;0mThe server is disconnected\033[0m\n";
char CHAT_LOAD_WITH_NEW_MESSAGES[] = "\033[38;2;0;255;0mUser authenticated, loading the chat with \033[4mnew messages!\033[0m";

void handle_new_messages(clca::msg::Message newMessage, bool &notified, clca::Chat &chat, condition_variable &cv, mutex &m1, int *newmsg)
{
    if (strcmp(newMessage.getContent(), "\0") == 0)
    {
        notified = true;
        cv.notify_one();
        m1.unlock();
        return;
    }

    chat.addMessageToQueue(newMessage);

    (void)--(*newmsg);

    if (*newmsg == 0)
    {
        notified = true;
        cv.notify_one();
        m1.unlock();
        delete (newmsg);
    }

    m1.unlock();
}

void handle_message(clca::msg::Message newMessage, clca::Chat &chat, mutex &m1, string input)
{
    chat.addMessage(newMessage);
    cout << "\033[G\033[K";
    newMessage.print();
    cout << "You:\033[s" << input << flush;
    m1.unlock();
}

void handle_disconnect_message(clca::Chat &chat, mutex &m1, string input, int &connection_flag, char message[])
{
    connection_flag = DISCONNECT;
    cout << "\033[G\033[K" << message;
    cout << "You:\033[s" << input << flush;
    m1.unlock();
}

void srv::client_listen_reicvmessage()
{
    Session *session = Session::getInstance();

    int *newmsg;

    int result;

    vector<clca::msg::Message> queue;

    vector<char> msg(MESSAGE_MAX_SIZE);

    size_t msg_offset = 0;

    while (1)
    {
        if (queue.empty())
        {

            result = recv(session->remote_socket, &msg[msg_offset], MESSAGE_MAX_SIZE - msg_offset, 0);
            
            /*if(result == -1){
                cerr << "Error during connection! Error code: "<< WSAGetLastError();
                exit(-1);
            }*/

            vector<char>::iterator it = msg.begin();

            while (1)
            {
                unique_ptr<clca::msg::Message> message(clca::msg::Message::fetchMessageFromString(it, msg.end()));

                if (message == nullptr)
                {
                    msg.erase(msg.begin(), it);
                    msg_offset = (*it == '\0') ? 0 : msg.size();
                    msg.resize(MESSAGE_MAX_SIZE);
                    break;
                }

                queue.insert(queue.begin(), *message);
            }
        }

        clca::msg::Message servermessage = *(queue.end() - 1);
        queue.pop_back();

        session->m1.lock();

        if (session->remote_connect == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the server quit
            session->m1.unlock();
            break;
        }

        if (result > 0)
        {
            switch (servermessage.getType())
            {
            case clca::msg::Type::AUTH:

#ifdef _WIN32
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                session->remote_username = servermessage.getOwner();
                session->remote_uuid = converter.from_bytes(servermessage.getContent());
            }
#elif __linux__
            {
                session->remote_username = servermessage.getOwner();
                session->remote_uuid = servermessage.getContent();
            }
#endif
                session->m1.unlock();
                break;
            case clca::msg::Type::INFO:
                servermessage.setOwner("Server");
                servermessage.print();

                if ((servermessage.getContent() == nullptr) || (servermessage.getContent()[0] == '\0'))
                {
                    goto notify;
                }

                if (strcmp(strtok(servermessage.getContent(), "-"), CHAT_LOAD_WITH_NEW_MESSAGES) == 0)
                    newmsg = new int(stoi(strtok(NULL, "-")));

            notify:

                session->notified = true;
                session->cv.notify_one();
                session->m1.unlock();
                break;
            case clca::msg::Type::MESSAGE:
                handle_message(servermessage, session->chat, session->m1, session->input);
                break;
            case clca::msg::Type::NEW_MESSAGE:
                handle_new_messages(servermessage, session->notified, session->chat, session->cv, session->m1, newmsg);
                break;
            }
        }
        else
        {
            handle_disconnect_message(session->chat, session->m1, session->input, session->remote_connect, SERVER_DISCONNECT);
            break;
        }
    }
}

void srv::server_listen_reicvmessage()
{
    Session *session = Session::getInstance();

    int *newmsg;

    int result;

    vector<clca::msg::Message> queue;

    vector<char> msg(MESSAGE_MAX_SIZE);

    size_t msg_offset = 0;

    while (1)
    {
        if (queue.empty())
        {

            result = recv(session->remote_socket, &msg[msg_offset], MESSAGE_MAX_SIZE - msg_offset, 0);

            /*if(result == -1){
                cerr << "Error during connection! Error code: "<< WSAGetLastError();
                exit(-1);
            }*/

            vector<char>::iterator it = msg.begin();

            while (1)
            {
                unique_ptr<clca::msg::Message> message(clca::msg::Message::fetchMessageFromString(it, msg.end()));

                if (message == nullptr)
                {
                    msg.erase(msg.begin(), it);
                    msg_offset = (*it == '\0') ? 0 : msg.size();
                    msg.resize(MESSAGE_MAX_SIZE);
                    break;
                }

                queue.insert(queue.begin(), *message);
            }
        }

        clca::msg::Message clientmessage = *(queue.end() - 1);
        queue.pop_back();

        session->m1.lock();

        if (session->remote_connect == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the client quit
            session->m1.unlock();
            break;
        }

        if (result > 0)
        {
            switch (clientmessage.getType())
            {
            case clca::msg::Type::AUTH:

#ifdef _WIN32
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                session->remote_username = clientmessage.getOwner();
                session->remote_uuid = converter.from_bytes(clientmessage.getContent());
            }
#elif __linux__
            {
                session->remote_username = clientmessage.getOwner();
                session->remote_uuid = clientmessage.getContent();
            }
#endif
                session->notified = true;
                session->cv.notify_one();
                session->m1.unlock();
                break;
            case clca::msg::Type::INFO:

                if (clientmessage.getContent()[0] != '\0')
                {
                    if (strcmp(strtok(clientmessage.getContent(), "-"), "new") == 0)
                        newmsg = new int(stoi(strtok(NULL, "-")));
                }

                session->m1.unlock();
                break;
            case clca::msg::Type::MESSAGE:
                handle_message(clientmessage, session->chat, session->m1, session->input);
                break;
            case clca::msg::Type::NEW_MESSAGE:
                handle_new_messages(clientmessage, session->notified, session->chat, session->cv, session->m1, newmsg);
                break;
            }
        }
        else
        {
            handle_disconnect_message(session->chat, session->m1, session->input, session->remote_connect, CLIENT_DISCONNECT);
            break;
        }
    }
}

int srv::start_session()
{
    Session *session = Session::getInstance();
    clca::Chat &chat = session->chat;
    string &input = session->input;
    mutex &m1 = session->m1;
    _SOCKET socket = session->remote_socket;
    string username = session->username;
    int &peer_connect = session->remote_connect;
    basic_string<_PATH_CHAR> &peer_uuid = session->remote_uuid;

    do
    {
        input = "";
        cout << "\033[s";

        cin.clear();

        while (!clca::msg::message_is_ready(input, username))
        {
        }

        m1.lock();

        if (input.length() > 0)
        {
            if (input == "quit")
            {
                peer_connect = DISCONNECT;
                _CLOSE_SOCKET(socket);
                m1.unlock();

                clca::save_chat(chat, peer_uuid);
                peer_uuid.clear();
                chat.clear();
                return EXIT_SUCCESS;
            }
            else
            {
                clca::msg::Message ownmessage = srv::send_message((peer_connect != CONNECT) ? clca::msg::Type::NEW_MESSAGE : clca::msg::Type::MESSAGE, username.c_str(), input.c_str());

                chat.addMessage(ownmessage);
                cout << "\033[G\033[J";
                ownmessage.print();
            }
        }
        else
        {
            cout << "\033[G\033[K";
        }

        m1.unlock();

        cout << "You:";
    } while (1);
}

void srv::send_new_message()
{
    Session *session = Session::getInstance();
    clca::Chat &chat = session->chat;
    size_t chat_size = chat.getSize();
    int file_flag = session->file_flag;
    string username = session->username;

    if (file_flag > 0)
    {
        for (size_t i = chat_size - file_flag; i < chat_size; i++)
        {
            chat.getAt(i)._send(session->remote_socket);
            chat.getAt(i).setType(clca::msg::Type::MESSAGE);
        }
    }
    else
    {
        srv::send_message(clca::msg::NEW_MESSAGE, username.c_str(), "");
    }

    chat.consumeQueueMessages();
}

void srv::wait_peer()
{
    Session *session = Session::getInstance();
    unique_lock<mutex> ul(session->m1);
    bool &notified = session->notified;
    session->cv.wait(ul, [&notified]
                     { return notified; });
    notified = false;
}