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

#ifndef CACHING_H
#define CACHING_H

#ifdef _WIN32
#include <winsock2.h>
#include <shlobj.h>
#include <ws2tcpip.h>
#include <conio.h>
typedef SOCKET _SOCKET;
#define __MAX_PATH MAX_PATH
#define _STR_COPY wcscpy
#define _STR_LEN wcslen
#define _STR_CAT wcscat
typedef wchar_t _PATH_CHAR;
#elif __linux
#include <limits.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
typedef SOCKET _SOCKET;
#define __MAX_PATH PATH_MAX
#define _STR_COPY strcpy
#define _STR_LEN strlen
#define _STR_CAT strcat
typedef char _PATH_CHAR;
#endif

#include <filesystem>
#include <fstream>
#include <list>
#include <vector>
#include <iostream>
#include <string.h>
#include <sstream>

#define BUFSIZE 264
#define PATH_NOT_FOUND -1
#define FILE_ALREADY_EXISTS 0
#define FILE_NOT_ALREADY_EXISTS -1

using namespace std;

namespace clca
{

    namespace msg
    {
        class Message
        {
        protected:
            char text[BUFSIZE];

            char owner[16];

            time_t timestamp;

        public:
            enum Type
            {
                AUTH,
                MESSAGE,
                NEW_MESSAGE
            };

            Type type;

            Message(){};

            Message(const Message &other);

            explicit Message(Type t);

            Message &operator=(const Message &other);

            bool operator<(const Message &other) const;

            bool operator==(const Message &other) const;

            void setOwner(const char *owner);

            char *getOwner();

            time_t getTimestamp();

            string getDecodedTimestamp();

            char *getContent();

            Type getType();

            void setType(Type type);

            void setTimestamp(time_t timestamp);

            void appendText(const char *message);

            void _send(_SOCKET socket);

            void print();

            void normalize();
        };

        bool message_is_ready(string &input, string username);

    }

    class Chat
    {
    private:
        std::list<msg::Message> messages;

    public:
        Chat(){};

        void addMessage(msg::Message msg);

        void removeMessage(msg::Message msg);

        void clear();

        msg::Message *getAt(int index);

        void print();

        int getSize();
    };

    int load_chat(Chat &chat, const char *foldername, const char *filename, std::string &myname);

    int save_chat(clca::Chat chat, std::string username);
}

namespace srca
{
    // TODO
}

#endif