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

char CLIENT_DISCONNECT[] = "\033[38;2;255;0;0mThe client is disconnected\033[0m\n";
char SERVER_DISCONNECT[] = "\033[38;2;255;0;0mThe server is disconnected\033[0m\n";
char NEW_MESSAGES[] = "\033[38;2;255;255;0mNew messages!\033[0m\n";
char CHAT_LOAD_WITH_NEW_MESSAGES[] = "\033[38;2;0;255;0mUser authenticated, loading the chat with \033[4mnew messages!\033[0m";

void srv::client_listen_reicvmessage(_SOCKET local_socket, int &connection_flag, clca::Chat &chat, mutex &m1, basic_string<_PATH_CHAR> &serveruuid, condition_variable &cv, bool &notified, string &input)
{
    int *newmsg;

    int result;

    vector<clca::msg::Message> queue;

    vector<char> msg(305);

    size_t msg_offset=0;

    while (1)
    {
        if(queue.empty()){

            result = recv(local_socket, &msg[msg_offset], /**TODO*/305-msg_offset, 0);

            vector<char>::iterator it=msg.begin();

            while(1){
                unique_ptr<clca::msg::Message> message(clca::msg::Message::fetchMessageFromString(it,msg.end()));

                if(message == nullptr){
                    msg.erase(msg.begin(),it);
                    msg_offset = (*it == '\0') ? 0 : msg.size();
                    msg.resize(305);
                    break;
                }

                queue.insert(queue.begin(), *message);
            }

        }

        clca::msg::Message servermessage=*(queue.end()-1);
        queue.pop_back();

        m1.lock();

        if (connection_flag == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the server quit
            m1.unlock();
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
                serveruuid = converter.from_bytes(servermessage.getOwner());
            }
#elif __linux__
            {
                serveruuid = servermessage.getOwner();
            }
#endif

                servermessage.setOwner("Server");
                servermessage.print();

                if ((servermessage.getContent() == nullptr) || (servermessage.getContent()[0] == '\0'))
                {
                    goto notify;
                }

                if (strcmp(strtok(servermessage.getContent(), "-"), CHAT_LOAD_WITH_NEW_MESSAGES) == 0)
                    newmsg = new int(stoi(strtok(NULL, "-")));

            notify:

                notified = true;
                cv.notify_one();
                m1.unlock();
                break;
            case clca::msg::Type::INFO:
                cerr << "INFO message not handled by client" << endl;
                break; 
            case clca::msg::Type::MESSAGE:
                handle_message(servermessage, chat, m1, input);
                break;
            case clca::msg::Type::NEW_MESSAGE:
                handle_new_messages(servermessage, notified, chat, cv, m1, newmsg);
                break;
            }
        }
        else
        {
            handle_disconnect_message(chat, m1, input, connection_flag, SERVER_DISCONNECT);
            break;
        }
    }
}

void srv::server_listen_reicvmessage(_SOCKET acceptedSocket, int &connection_flag, clca::Chat &chat, mutex &m1, basic_string<_PATH_CHAR> &clientuuid, condition_variable &cv, bool &notified, string &input)
{

    int *newmsg;

    int result;

    vector<clca::msg::Message> queue;

    vector<char> msg(305);

    size_t msg_offset=0;

    while (1)
    {
        if(queue.empty()){

            result = recv(acceptedSocket, &msg[msg_offset], /**TODO*/305-msg_offset, 0);

            vector<char>::iterator it=msg.begin();

            while(1){
                unique_ptr<clca::msg::Message> message(clca::msg::Message::fetchMessageFromString(it,msg.end()));

                if(message == nullptr){
                    msg.erase(msg.begin(),it);
                    msg_offset = (*it == '\0') ? 0 : msg.size();
                    msg.resize(305);
                    break;
                }

                queue.insert(queue.begin(), *message);
            }

        }

        clca::msg::Message clientmessage=*(queue.end()-1);
        queue.pop_back();

        m1.lock();

        if (connection_flag == DISCONNECT)
        { // i use this because i wanna notify the user for the lost connection only if the client quit
            m1.unlock();
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
                clientuuid = converter.from_bytes(clientmessage.getOwner());
            }
#elif __linux__
            {
                clientuuid = clientmessage.getOwner();
            }
#endif

                cv.notify_one();
                m1.unlock();
                break;
            case clca::msg::Type::INFO:

                if (clientmessage.getContent()[0] != '\0')
                {
                    if (strcmp(strtok(clientmessage.getContent(), "-"), "new") == 0)
                        newmsg = new int(stoi(strtok(NULL, "-")));
                }

                m1.unlock();
                break;    
            case clca::msg::Type::MESSAGE:
                handle_message(clientmessage, chat, m1, input);
                break;
            case clca::msg::Type::NEW_MESSAGE:
                handle_new_messages(clientmessage, notified, chat, cv, m1, newmsg);
                break;
            }
        }
        else
        {
            handle_disconnect_message(chat, m1, input, connection_flag, CLIENT_DISCONNECT);
            break;
        }
    }
}

void srv::handle_new_messages(clca::msg::Message newMessage, bool &notified, clca::Chat &chat, condition_variable &cv, mutex &m1, int *newmsg)
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

void srv::handle_message(clca::msg::Message newMessage, clca::Chat &chat, mutex &m1, string input)
{
    chat.addMessage(newMessage);
    cout << "\033[G\033[K";
    newMessage.print();
    cout << "You:\033[s" << input << flush;
    m1.unlock();
}

void srv::handle_disconnect_message(clca::Chat &chat, mutex &m1, string input, int &connection_flag, char message[])
{
    connection_flag = DISCONNECT;
    cout << "\033[G\033[K" << message;
    cout << "You:\033[s" << input << flush;
    m1.unlock();
}