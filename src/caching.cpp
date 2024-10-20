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

#include "../include/caching.hpp"
#include "../include/connection.hpp"

basic_string<_PATH_CHAR> root_dir;
basic_string<_PATH_CHAR> cache_dir;
basic_string<_PATH_CHAR> auth_dir;
fstream chatcache;
int dirtyflag;

string _UUID()
{
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dis;

    uint64_t half1 = dis(gen);
    uint64_t half2 = dis(gen);

    half1 &= ~(0xFULL << 12);
    half1 |= (0x4ULL << 12); // Set the version to 4

    half2 &= ~(0x3ULL << 62);
    half2 |= (0x2ULL << 62); // Set the variant to 0b10

    stringstream uuidStream;

    uuidStream << hex << setw(16) << setfill('0') << half1 << setw(16) << setfill('0') << half2;

    return uuidStream.str();
}

int setRootDir()
{
    _PATH_CHAR *path;
#ifdef _WIN32
    if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &path)))
    { // get /user/appdata/localLow folder
        cout << "Directory not found!";
        system("pause");
        return PATH_NOT_FOUND;
    }
    else
    {
        for (size_t i = 0; i < wcslen(path); i++)
        { // switch \ to / because C:\..\..\.. is invalis path
            if (path[i] == '\\')
            {
                path[i] = '/';
            }
        }

        root_dir = path;
    }
#elif __linux__
    path = getenv("XDG_DATA_HOME");

    if (path == NULL || path[0] == '\0')
    { // XDG_DATA_HOME not set
        path = getenv("HOME");

        if (path == NULL || path[0] == '\0')
        { // HOME not set
            root_dir = "/tmp";
        }
        else
        {
            root_dir = path;
            root_dir.append("/.local/share");
        }
    }
    else
    {
        root_dir = path;
    }
#endif
    return PATH_FOUND;
}

const char* help_message =
    "Usage: SocketFalco [options] <arguments>\n\n"
    "Options:\n"
    "-l, -listen                   Start the server. Use -t for temporary session.\n"
    "                                Example: SocketFalco -l\n"
    "                                         SocketFalco -l -t\n\n"
    "-ls, -list                    List available chat sessions.\n"
    "                                Example: SocketFalco -ls\n\n"
    "-c, -connect <address>        Connect to a specified server address. Use -t for temporary session.\n"
    "                                Example: SocketFalco -c 192.168.1.1\n"
    "                                         SocketFalco -c -t 192.168.1.1\n\n"
    "-eN, -editName <name>         Edit the current user name.\n"
    "                                Example: SocketFalco -eN NewName\n\n"
    "-eC, -editChat <ChatUUID>     Open the specific chat\n"
    "                                Example: SocketFalco -eC chatUUID\n\n"
    "-h, -help                     Show this help message and exit.\n"
    "\n"
    "Examples:\n"
    "SocketFalco -listen                   Start the server.\n"
    "SocketFalco -listen -t                Start the server in temporary mode.\n"
    "SocketFalco -connect 192.168.0.1      Connect to the server at 192.168.0.1.\n"
    "SocketFalco -connect -t 192.168.0.1   Connect to the server at 192.168.0.1 in temporary mode.\n"
    "SocketFalco -list                     List all chat sessions.\n"
    "SocketFalco -editName Alice           Change the username to \"Alice\".\n"
    "SocketFalco -editChat 97b8b...        Open the specific chat and let the user to add new messages.\n";

namespace clca
{
    void Chat::addMessage(msg::Message msg)
    {
        this->messages.insert(msg);
    }

    void Chat::removeMessage(msg::Message msg)
    {
        this->messages.extract(msg);
    }

    void Chat::addMessageToQueue(msg::Message msg)
    {
        this->queue.push_back(msg);
    }

    void Chat::consumeQueueMessages()
    {
        for (auto it = this->queue.begin(); it != this->queue.end(); ++it)
        {
            this->addMessage(*it);
        }

        this->queue.clear();
    }

    void Chat::clearQueue()
    {
        this->queue.clear();
    }

    void Chat::clear()
    {
        this->messages.clear();
    }

    msg::Message &Chat::getAt(int index)
    {
        auto l_front = messages.begin();

        advance(l_front, index);

        return const_cast<msg::Message &>(*l_front);
    }

    void Chat::print(bool normalize)
    {
        for (auto it = this->messages.begin(); it != this->messages.end(); ++it)
        {
            msg::Message &msg = const_cast<msg::Message &>(*it);
            msg.print();

            if (normalize)
                msg.normalize();
        }
    }

    size_t Chat::getSize()
    {
        return this->messages.size();
    }

    int load_chat(Chat &chat, basic_string<_PATH_CHAR> filename)
    {
        basic_string<_PATH_CHAR> filedir = getCacheDir() + _STR_FORMAT(/) + filename;

        Session* session = Session::getInstance();

        if (!(chatcache = fstream(filedir.c_str())))
        {
            chatcache.open(filedir.c_str(), fstream::in | fstream::out | fstream::trunc);
            dirtyflag = FILE_NOT_ALREADY_EXISTS;
        }
        else
        {
            dirtyflag = FILE_ALREADY_EXISTS;
            string line;

            //i get here the remote username from old session, during the new authentication(if it will occurr) this will be overwrited
            getline(chatcache, session->remote_username);
            chatcache.ignore(IPSIZE+1, '\n');

            while (getline(chatcache, line))
            {
                msg::Message message(msg::Type::MESSAGE);

                vector<char> cstr(line.c_str(), line.c_str() + line.size() + 1);

                message.setTimestamp(stol(strtok(cstr.data(), "\xB2")));

                message.setOwner(strcmp(strtok(NULL, "\xB2"), "you") != 0 ? session->remote_username.c_str() : session->username.c_str());

                if (strcmp(strtok(NULL, "\xB2"), "new") == 0)
                {
                    message.setType(msg::Type::NEW_MESSAGE);
                    dirtyflag++;
                }

                message.appendText(strtok(NULL, "\xB2"));

                chat.addMessage(message);
            }
        }
        chatcache.close();
        chatcache.clear();

        return dirtyflag;
    }

    int save_chat(Chat chat, basic_string<_PATH_CHAR> filename)
    {
        basic_string<_PATH_CHAR> filedir = getCacheDir() + _STR_FORMAT(/) + filename;

        chatcache.open(filedir.c_str(), fstream::in | fstream::out | fstream::trunc); // TODO create another file every time(it's not good for long chat)

        Session* session = Session::getInstance();

        chatcache << session->remote_username << endl;
        chatcache << std::setfill('0') << std::setw(IPSIZE) << int(
#ifdef _WIN32
                                                                   session->remote_socket_addr.sin_addr.S_un.S_addr
#elif __linux__
                                                                   session->remote_socket_addr.sin_addr.s_addr
#endif
        ) << endl;

        for (size_t i = 0; i < chat.getSize(); i++)
        {
            msg::Message &msg = chat.getAt(i);

            chatcache << msg.getTimestamp() << "\xB2" << (strcmp(msg.getOwner(), session->remote_username.c_str()) == 0 ? msg.getOwner() : "you" )<< "\xB2" << ((msg.getType() == msg::Type::NEW_MESSAGE) ? "new" : "wen") << "\xB2" << msg.getContent()
                      << "\n";
        }

        chatcache.close();
        chatcache.clear();

        return 1;
    }

    int list_chat(){
        auto path = clca::getCacheDir();

        std::cout << "  ID\t\t\t\t\tNAME\t\tIP\n";

        size_t c = 1;
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            fstream f(entry.path().c_str(), fstream::in);

            string username,ip_str;
            f >> username;
            f >> ip_str;

            IN_ADDR ip_addr;
            ip_addr.S_un.S_addr =  std::atoi(ip_str.c_str());
            char *ip = inet_ntoa(ip_addr);

            username.resize(15, ' ');

            std::cout << c++ << ">"
                      << entry.path().filename().string()
                      << "\t" << username
                      << "\t" << ip << std::endl;
        }

        return EXIT_SUCCESS;
    }

    int show_help(){
        std::cout << help_message << endl;
        return EXIT_SUCCESS;
    }

    int update_name(string name)
    {
        if (name.length() > 15)
        {
            cout << name << " is too long(max 15 characters)" << endl;
            return EXIT_FAILURE;
        }
        else if (name.length() < 2)
        {
            cout << "at least two character!" << endl;
            return EXIT_FAILURE;
        }

        fstream f((getAuthDir() + _STR_FORMAT(/uuid)).c_str(), fstream::in | fstream::out);

        string oldname, uuid;

        getline(f, oldname);
        getline(f, uuid);

        f << name << endl;
        f << (uuid.empty() ? clca::genUUID(name) : uuid) << endl;

        f.close();

        cout << "\033[38;2;255;255;0mUsername succesfully updated!\033[0m\n";
        cout << oldname << " -> " << name << endl;
        return EXIT_SUCCESS;
    }

    int loadUUID(int type, string &myname, string &uuid)
    {
        basic_string<_PATH_CHAR> filedir = getAuthDir() + _STR_FORMAT(/uuid);

        if (!(chatcache = fstream(filedir.c_str())))
        {
            chatcache.open(filedir.c_str(), fstream::in | fstream::out | fstream::trunc);
            dirtyflag = FILE_NOT_ALREADY_EXISTS;
        }
        else
        {
            dirtyflag = FILE_ALREADY_EXISTS;

            getline(chatcache, myname);
            getline(chatcache, uuid);
        }
        chatcache.close();
        chatcache.clear();

        return dirtyflag;
    }

    string genUUID(string username)
    {
        string uuid = _UUID();

        basic_string<_PATH_CHAR> filedir = getAuthDir() + _STR_FORMAT(/uuid);

        chatcache.open(filedir.c_str(), fstream::in | fstream::out | fstream::trunc);

        chatcache << username + "\n";
        chatcache << uuid;

        chatcache.close();
        chatcache.clear();

        return uuid;
    }

    int fileSysSetup()
    {
        if (setRootDir() == PATH_NOT_FOUND)
            return EXIT_FAILURE;

        root_dir = root_dir + _STR_FORMAT(/SocketFalco);

        try
        {
            if (filesystem::create_directory(root_dir))
            {
                _STR_COUT << _STR_FORMAT(\033[38;2;255;255;0mDirectory \033[4m)<<root_dir<<_STR_FORMAT(\033[0m\033[38;2;255;255;0m created!\033[0m\n);
            }
        }
        catch (exception &e)
        {
            cout << e.what() << endl;
            return EXIT_FAILURE;
        }

        auth_dir = root_dir + _STR_FORMAT(/auth);

        try
        {
            if (filesystem::create_directory(auth_dir))
            {
                _STR_COUT << _STR_FORMAT(\033[38;2;255;255;0mDirectory \033[4m)<<auth_dir<<_STR_FORMAT(\033[0m\033[38;2;255;255;0m created!\033[0m\n);
            }
        }
        catch (exception &e)
        {
            cout << e.what() << endl;
            return EXIT_FAILURE;
        }

        cache_dir = root_dir + _STR_FORMAT(/cache);

        try
        {
            if (filesystem::create_directory(cache_dir))
            {
                _STR_COUT << _STR_FORMAT(\033[38;2;255;255;0mDirectory \033[4m)<<cache_dir<<_STR_FORMAT(\033[0m\033[38;2;255;255;0m created!\033[0m\n);
            }
        }
        catch (exception &e)
        {
            cout << e.what() << endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    basic_string<_PATH_CHAR> getRootDir()
    {
        if (root_dir.empty())
        {
            if (fileSysSetup() == EXIT_SUCCESS)
                return root_dir;
            else
                return _STR_FORMAT();
        }

        return root_dir;
    }

    basic_string<_PATH_CHAR> getCacheDir()
    {
        if (cache_dir.empty())
        {
            if (fileSysSetup() == EXIT_SUCCESS)
                return cache_dir;
            else
                return _STR_FORMAT();
        }

        return cache_dir;
    }

    basic_string<_PATH_CHAR> getAuthDir()
    {
        if (auth_dir.empty())
        {
            if (fileSysSetup() == EXIT_SUCCESS)
                return auth_dir;
            else
                return _STR_FORMAT();
        }

        return auth_dir;
    }

    namespace msg
    {
        Message::Message(const Message &other) : timestamp(other.timestamp), type(other.getType())
        {
            strncpy(this->owner, other.owner, OWNERZIZE);
            strncpy(this->text, other.text, BUFSIZE);
        }

        Message::Message()
        {
            memset(this->text, '\0', BUFSIZE);
            memset(this->owner, '\0', OWNERZIZE);
        }

        Message::Message(Type t) : text{}, owner{}, type(t)
        {
            this->timestamp = chrono::system_clock::to_time_t(chrono::system_clock::now());
        };

        Message &Message::operator=(const Message &other)
        {
            if (&other == this)
            {
                return *this;
            }

            strncpy(this->text, other.text, OWNERZIZE);
            this->type = other.type;
            this->timestamp = other.timestamp;

            return *this;
        }

        Message *Message::fetchMessageFromString(vector<char>::iterator &str, const vector<char>::iterator &end)
        {
            Message *msg = new Message();

            size_t text_size, own_size;

            const vector<char>::iterator str_copy = str;

            if (*str == '\0')
                return nullptr;

            vector<char> input(8 + 1); // data size * 2(because one char is half-byte(F)) + \0
            copy(str, str + input.size() - 1, input.begin());
            msg->setTimestamp(strtol(&input[0], NULL, 16));

            if ((str + input.size() - 1) < end)
            {
                str += input.size() - 1;
            }
            else
            {
                return nullptr;
            }

            input.clear();

            input.resize(4 + 1); // data size * 2(because one char is half-byte(F)) + \0
            copy(str, str + input.size() - 1, input.begin());
            msg->setType(static_cast<Type>(strtol(&input[0], NULL, 16)));

            if ((str + input.size() - 1) < end)
            {
                str += input.size() - 1;
            }
            else
            {
                str = str_copy;
                return nullptr;
            }

            input.clear();

            input.resize(2 + 1); // data size * 2(because one char is half-byte(F)) + \0
            copy(str, str + input.size() - 1, input.begin());
            text_size = strtol(&input[0], NULL, 16);

            if ((str + input.size() - 1) < end)
            {
                str += input.size() - 1;
            }
            else
            {
                str = str_copy;
                return nullptr;
            }

            input.clear();

            input.resize(2 + 1); // data size * 2(because one char is half-byte(F)) + \0
            copy(str, str + input.size() - 1, input.begin());
            own_size = strtol(&input[0], NULL, 16);

            if ((str + input.size() - 1) < end)
            {
                str += input.size() - 1;
            }
            else
            {
                str = str_copy;
                return nullptr;
            }

            input.clear();

            if (text_size == 0)
                goto no_text;

            input.resize(text_size + 1); // data size + \0
            copy(str, str + text_size, input.begin());
            msg->appendText(&input[0]);

            if ((str + text_size) < end)
            {
                str += text_size;
            }
            else
            {
                str = str_copy;
                return nullptr;
            }

            input.clear();

        no_text:

            if (own_size == 0)
                goto no_own;

            input.resize(own_size + 1); // data size + \0
            copy(str, str + own_size, input.begin());
            msg->setOwner(&input[0]);

            if ((str + own_size + 1) < end)
            {
                str += own_size + 1;
            }
            else
            {
                str = str_copy;
                return nullptr;
            }

            input.clear();

        no_own:

            return msg;
        }

        bool Message::operator<(const Message &other) const
        {
            return this->timestamp < other.timestamp;
        }

        bool Message::operator==(const Message &other) const
        {
            return this->timestamp == other.timestamp;
        }

        void Message::setOwner(const char *owner)
        {
            strncpy(this->owner, owner, OWNERZIZE);
        }

        char *Message::getOwner()
        {
            return this->owner;
        }

        time_t Message::getTimestamp()
        {
            return this->timestamp;
        }

        string Message::getDecodedTimestamp()
        {

            tm *timeInfo = localtime(&timestamp);

            stringstream tms;

            tms << "[" << timeInfo->tm_mday
                << "/" << timeInfo->tm_mon + 1
                << "/" << (timeInfo->tm_year + 1900) << " "
                << timeInfo->tm_hour << ":" << ((timeInfo->tm_min < 10) ? "0" : "")
                << timeInfo->tm_min << "]";

            return tms.str();
        }

        void Message::setType(msg::Type type)
        {
            this->type = type;
        }

        void Message::setTimestamp(time_t timestamp)
        {
            this->timestamp = timestamp;
        }

        void Message::appendText(const char *message)
        {
            strcat(this->text, message);
        };

        void Message::_send(_SOCKET socket)
        {
            string msg = this->parseString();
            send(socket, msg.c_str(), msg.length() + 1, 0);
        };

        string Message::parseString()
        {
            ostringstream _str;

            _str << hex // i put hex value just for readability
                 << setw(8) << setfill('0') << (0xFFFFFFFF & this->getTimestamp())
                 << setw(4) << setfill('0') << (0xFFFF & this->getType())
                 << setw(2) << setfill('0') << (0xFF & strlen(this->text))
                 << setw(2) << setfill('0') << (0xFF & strlen(this->owner))
                 << this->text
                 << this->owner;

            return _str.str();
        }

        char *Message::getContent()
        {
            return this->text;
        }

        msg::Type Message::getType() const
        {
            return this->type;
        }

        void Message::print()
        {
            cout << this->getDecodedTimestamp() << " " << this->getOwner() << ": " << this->getContent() << ((this->type == msg::Type::NEW_MESSAGE) ? "\033[38;2;255;255;0m \xFEnew\xFE\033[0m" : "") << endl;
        }

        void Message::normalize()
        {
            if (this->type == msg::Type::NEW_MESSAGE)
                this->setType(msg::Type::MESSAGE);
        }

        bool message_is_ready(string &input, string username)
        {
            char ch = 0;
#ifdef _WIN32

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

            cout << "\033[u\033[K" << input;

#elif __linux__
            struct termios oldt, newt;

            // Save current terminal settings
            tcgetattr(STDIN_FILENO, &oldt);

            // Set terminal to non-blocking mode
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);

            // Attempt to read a character
            while ((ch = getchar()) == EOF)
            {
            }

            if (ch == '\n')
                return true;

            if ((ch != '\b') && (input.length() < BUFSIZE - (username.size() + 3))) // 3 is the size of ":"+"\n"+"\0"
                input += ch;

            if ((ch == '\b') && (input.size() > 0))
                input.pop_back();

            cout << "\033[u\033[J" << input;

            // Restore old terminal settings
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

            return false;
        }
    }
}
