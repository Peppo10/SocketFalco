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

void clca::Chat::addMessage(clca::msg::Message msg)
{
    this->messages.push_back(msg);
}

void clca::Chat::removeMessage(clca::msg::Message msg)
{
    this->messages.remove(msg);
}

void clca::Chat::clear()
{
    this->messages.clear();
}

clca::msg::Message *clca::Chat::getAt(int index)
{
    auto l_front = messages.begin();

    advance(l_front, index);

    return &(*l_front);
}

void clca::Chat::print()
{
    for (clca::msg::Message &msg : this->messages)
    {
        msg.print();
    }
}

int clca::Chat::getSize()
{
    return this->messages.size();
}

int clca::load_chat(Chat &chat, const char *foldername, const char *filename, std::string &myname)
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
            if (std::filesystem::create_directory(path))
            {
                cout << "directory created!";
            }
        }
        catch (std::exception &e)
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
                clca::msg::Message message(clca::msg::Message::MESSAGE);

                vector<char> cstr(line.c_str(), line.c_str() + line.size() + 1);

                message.setTimestamp(stol(strtok(cstr.data(), "\xB2")));

                message.setOwner(strtok(NULL, "\xB2"));

                if (strcmp(strtok(NULL, "\xB2"), "new") == 0)
                {
                    message.setType(clca::msg::Message::NEW_MESSAGE);
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

int clca::save_chat(clca::Chat chat, std::string username)
{
    chatcache.open(path, fstream::in | fstream::out | fstream::trunc); // TODO create another file every time(it's not good for long chat)
    chatcache << username + "\n";

    for (int i = 0; i < chat.getSize(); i++)
    {
        clca::msg::Message msg = *chat.getAt(i);

        chatcache << msg.getTimestamp() << "\xB2" << msg.getOwner() << "\xB2" << ((msg.getType() == clca::msg::Message::NEW_MESSAGE) ? "new" : "wen") << "\xB2" << msg.getContent()
                  << "\n";
    }

    chatcache.close();
    chatcache.clear();

    return 1;
}

clca::msg::Message::Message(const clca::msg::Message &other) : timestamp(other.timestamp), type(other.type)
{
    strcpy(this->owner, other.owner);
    strcpy(this->text, other.text);
}

clca::msg::Message::Message(Type t) : text{}, owner{}, type(t)
{
    this->timestamp = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
};

clca::msg::Message &clca::msg::Message::operator=(const clca::msg::Message &other)
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

bool clca::msg::Message::operator<(const clca::msg::Message &other) const
{
    return this->timestamp < other.timestamp;
}

bool clca::msg::Message::operator==(const clca::msg::Message &other) const
{
    return this->timestamp == other.timestamp;
}

void clca::msg::Message::setOwner(const char *owner)
{
    strcpy(this->owner, owner);
}

char *clca::msg::Message::getOwner()
{
    return this->owner;
}

time_t clca::msg::Message::getTimestamp()
{
    return this->timestamp;
}

string clca::msg::Message::getDecodedTimestamp()
{

    tm *timeInfo = localtime(&timestamp);

    stringstream tms;

    tms << "[" << timeInfo->tm_mday << "/" << timeInfo->tm_mon + 1 << "/" << (timeInfo->tm_year % 100) << " " << timeInfo->tm_hour << ":" << timeInfo->tm_min << "]";
    return tms.str();
}

void clca::msg::Message::setType(clca::msg::Message::Type type)
{
    this->type = type;
}

void clca::msg::Message::setTimestamp(time_t timestamp)
{
    this->timestamp = timestamp;
}

void clca::msg::Message::appendText(const char *message)
{
    strcat(this->text, message);
};

void clca::msg::Message::_send(_SOCKET socket)
{
    send(socket, (char *)this, sizeof(Message), 0);
};

char *clca::msg::Message::getContent()
{
    return this->text;
}

clca::msg::Message::Type clca::msg::Message::getType()
{
    return this->type;
}

void clca::msg::Message::print()
{
    cout << this->getDecodedTimestamp() << " " << this->getOwner() << ": " << this->getContent() << ((this->type == clca::msg::Message::NEW_MESSAGE) ? "\033[38;2;255;255;0m \xFEnew\xFE\033[0m" : "") << endl;
    this->normalize();
}

void clca::msg::Message::normalize()
{
    if (this->type == clca::msg::Message::NEW_MESSAGE)
        this->setType(clca::msg::Message::MESSAGE);
}

bool clca::msg::message_is_ready(string &input, string username)
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
