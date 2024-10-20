#include "../include/connection.hpp"
#include "../include/service.hpp"

int add_new_messages(string remote)
{
    [[maybe_unused]] string uuid;
    Session* offlineSession = Session::getInstance();

#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    wstring remote_uuid = converter.from_bytes(remote.c_str());
#elif __linux__
    string remote_uuid = remote;
#endif

    if ((clca::loadUUID(0, offlineSession->username, offlineSession->uuid) != FILE_NOT_ALREADY_EXISTS) && (clca::load_chat(offlineSession->chat, remote_uuid) != FILE_NOT_ALREADY_EXISTS))
    {
        string username = offlineSession->username;
        string &input = offlineSession->input;
        clca::Chat &chat = offlineSession->chat;

        chat.print(false);
        cout << "You:";

        do
        {
            input = "";
            cout << "\033[s";

            cin.clear();

            while (!clca::msg::message_is_ready(input, username))
            {
            }

            if (input.length() > 0)
            {
                if (input == "quit")
                {

                    clca::save_chat(chat, remote_uuid);
                    return EXIT_SUCCESS;
                }
                else
                {
                    clca::msg::Message ownmessage = srv::send_message(clca::msg::Type::NEW_MESSAGE, username.c_str(), input.c_str());

                    chat.addMessage(ownmessage);
                    cout << "\033[G\033[J";
                    ownmessage.print();
                }
            }
            else
            {
                cout << "\033[G\033[K";
            }

            cout << "You:";
        } while (1);

        return EXIT_SUCCESS;
    }

    _STR_COUT << "chat with " << remote_uuid << " not found!" << endl;

    return EXIT_FAILURE;
}

Session *Session::instance = NULL;

Session *Session::getInstance()
{
    if (Session::instance == NULL)
    {
        Session::instance = new Session();
        return Session::instance;
    }

    return Session::instance;
}

void Session::clearInstance()
{
    if (Session::instance != NULL){
        Session::instance->chat.clearQueue();
        Session::instance->input = "";
        Session::instance->uuid = "";
        Session::instance->username = "";
        Session::instance->remote_uuid = _STR_FORMAT("");
        Session::instance->remote_username = "";
    }
}