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

#include "caching.hpp"

_PATH_CHAR path[__MAX_PATH];
_PATH_CHAR *path_ref = NULL;
fstream chatcache;
int dirtyflag;

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

    void Chat::print()
    {
        for (auto it = this->messages.begin(); it != this->messages.end(); ++it)
        {
            msg::Message &msg = const_cast<msg::Message &>(*it);
            msg.print();
        }
    }

    int Chat::getSize()
    {
        return this->messages.size();
    }

    int load_chat(Chat &chat, const char *foldername, const char *filename, string &myname)
    {
#ifdef _WIN32
        if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &path_ref)))
        { // get /user/appdata/localLow folder
            cout << "Directory not found!";
            system("pause");
            return PATH_NOT_FOUND;
        }
        else
        {
#elif __linux__
        if (path_ref == NULL)
        {
            path_ref = getenv("XDG_DATA_HOME");

            if (path_ref == NULL || path_ref[0] == '\0')
            { // XDG_DATA_HOME not set
                path_ref = getenv("HOME");

                if (path_ref == NULL || path_ref[0] == '\0')
                { // HOME not set
                    path_ref = "/tmp";
                }
                else
                {
                    strcat(path_ref, "/.local/share");
                }
            }
        }
#endif
            _STR_COPY(path, path_ref);
            for (size_t i = 0; i < _STR_LEN(path); i++)
            { // switch \ to / because C:\..\..\.. is invalis path
                if (path[i] == '\\')
                {
                    path[i] = '/';
                }
            }

            _PATH_CHAR *wc = new _PATH_CHAR[strlen(foldername) + 1];

#ifdef _WIN32
            mbstowcs(wc, foldername, strlen(foldername) + 1);
#elif __linux__
        _STR_COPY(wc, foldername);
#endif

            _STR_CAT(path, wc);

            try
            {
                if (filesystem::create_directory(path))
                {
                    cout << "directory created!";
                }
            }
            catch (exception &e)
            {
                cout << e.what() << endl;
            }

            wc = new _PATH_CHAR[strlen(filename) + 1];

#ifdef _WIN32
            mbstowcs(wc, filename, strlen(filename) + 1);
#elif __linux__
        _STR_COPY(wc, filename);
#endif

            _STR_CAT(path, wc);

            if (!(chatcache = fstream(path)))
            {
                chatcache.open(path, fstream::in | fstream::out | fstream::trunc);
                dirtyflag = FILE_NOT_ALREADY_EXISTS;
            }
            else
            {
                dirtyflag = FILE_ALREADY_EXISTS;
                string line;

                getline(chatcache, myname);

                while (getline(chatcache, line))
                {
                    msg::Message message(msg::Message::MESSAGE);

                    vector<char> cstr(line.c_str(), line.c_str() + line.size() + 1);

                    message.setTimestamp(stol(strtok(cstr.data(), "\xB2")));

                    message.setOwner(strtok(NULL, "\xB2"));

                    if (strcmp(strtok(NULL, "\xB2"), "new") == 0)
                    {
                        message.setType(msg::Message::NEW_MESSAGE);
                        dirtyflag++;
                    }

                    message.appendText(strtok(NULL, "\xB2"));

                    chat.addMessage(message);
                }
            }
            chatcache.close();
            chatcache.clear();
#ifdef _WIN32
        }
#endif

        return dirtyflag;
    }

    int save_chat(Chat chat, string username)
    {
        chatcache.open(path, fstream::in | fstream::out | fstream::trunc); // TODO create another file every time(it's not good for long chat)
        chatcache << username + "\n";

        for (int i = 0; i < chat.getSize(); i++)
        {
            msg::Message &msg = chat.getAt(i);

            chatcache << msg.getTimestamp() << "\xB2" << msg.getOwner() << "\xB2" << ((msg.getType() == msg::Message::NEW_MESSAGE) ? "new" : "wen") << "\xB2" << msg.getContent()
                      << "\n";
        }

        chatcache.close();
        chatcache.clear();

        return 1;
    }

    namespace msg
    {
        Message::Message(const Message &other) : timestamp(other.timestamp), type(other.type)
        {
            strcpy(this->owner, other.owner);
            strcpy(this->text, other.text);
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

            strcpy(this->text, other.text);
            this->type = other.type;
            this->timestamp = other.timestamp;

            return *this;
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
            strcpy(this->owner, owner);
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
                << "/" << (timeInfo->tm_year % 100) << " "
                << timeInfo->tm_hour << ":" << ((timeInfo->tm_min < 10) ? "0" : "")
                << timeInfo->tm_min << "]";

            return tms.str();
        }

        void Message::setType(Message::Type type)
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
            send(socket, (char *)this, sizeof(Message), 0);
        };

        char *Message::getContent()
        {
            return this->text;
        }

        Message::Type Message::getType()
        {
            return this->type;
        }

        void Message::print()
        {
            cout << this->getDecodedTimestamp() << " " << this->getOwner() << ": " << this->getContent() << ((this->type == Message::NEW_MESSAGE) ? "\033[38;2;255;255;0m \xFEnew\xFE\033[0m" : "") << endl;
            this->normalize();
        }

        void Message::normalize()
        {
            if (this->type == Message::NEW_MESSAGE)
                this->setType(Message::MESSAGE);
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

            cout << "\033[u\033[J" << input;

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
