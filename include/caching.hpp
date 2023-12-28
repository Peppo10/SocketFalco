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
#include <assert.h>
typedef SOCKET _SOCKET;
#define __MAX_PATH MAX_PATH
#define _STR_COPY wcscpy
#define _STR_LEN wcslen
#define _STR_CAT wcscat
#define _STR_FORMAT(str) L## #str
#define _STR_COUT wcout
typedef wchar_t _PATH_CHAR;
#elif __linux
#include <limits.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
typedef int _SOCKET;
#define __MAX_PATH PATH_MAX
#define _STR_COPY strcpy
#define _STR_LEN strlen
#define _STR_CAT strcat
#define _STR_FORMAT(str) #str
#define _STR_COUT cout
typedef char _PATH_CHAR;
#endif

#include <filesystem>
#include <fstream>
#include <set>
#include <vector>
#include <iostream>
#include <string.h>
#include <sstream>
#include <random>

#define BUFSIZE 255
#define OWNERZIZE 34
#define MESSAGE_MAX_SIZE BUFSIZE + OWNERZIZE + 8 + 4 + 2 + 2 + 1
#define PATH_NOT_FOUND -3
#define PATH_FOUND -2
#define FILE_ALREADY_EXISTS 0
#define FILE_NOT_ALREADY_EXISTS -1

using namespace std;

namespace clca
{

    namespace msg
    {

        enum Type{
                AUTH = 0xf000,
                INFO = 0x0f00,
                MESSAGE = 0x00f0,
                NEW_MESSAGE = 0x000f
        };

        
        class Message
        {
        private:
            char text[BUFSIZE];

            char owner[OWNERZIZE];

            time_t timestamp;

            Type type;

            string parseString();

        public:

            Message();

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

            Type getType() const;

            void setType(Type type);

            void setTimestamp(time_t timestamp);

            void appendText(const char *message);

            void _send(_SOCKET socket);

            void print();

            void normalize();

            static Message *fetchMessageFromString(vector<char>::iterator &str, const vector<char>::iterator &end); 
        };

        bool message_is_ready(string &input, string username);

    }

    class Chat
    {
    private:
        multiset<msg::Message> messages;

        vector<msg::Message> queue;

    public:
        Chat(){};

        void addMessageToQueue(msg::Message msg);

        void consumeQueueMessages();

        void clearQueue();

        void addMessage(msg::Message msg);

        void removeMessage(msg::Message msg);

        void clear();

        msg::Message &getAt(int index);

        void print();

        size_t getSize();
    };

    int load_chat(Chat &chat, basic_string<_PATH_CHAR> filename);

    int loadUUID(int type, string &myname, string &uuid);

    string genUUID(string username);

    int save_chat(clca::Chat chat, basic_string<_PATH_CHAR> filename);

    int fileSysSetup();

    basic_string<_PATH_CHAR> getRootDir();

    basic_string<_PATH_CHAR> getCacheDir();
    
    basic_string<_PATH_CHAR> getAuthDir();

    int update_name(string name);
}

#endif