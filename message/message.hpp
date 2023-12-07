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

#ifndef MESSAGE_H
#define MESSAGE_H

#ifdef _WIN32
#include <conio.h>
#elif __linux__
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define BUFSIZE 264

#include <iostream>

using namespace std;

namespace msg
{
    class Message
    {
    private:
        char text[BUFSIZE];

    public:
        enum Type
        {
            AUTH,
            MESSAGE,
            NEW_MESSAGE
        };

        Type type;

        Message(){};

        Message(const Message &other) : type(other.type)
        {
            strcpy(this->text, other.text);
        }

        Message &operator=(const Message &other)
        {
            if (&other == this)
            {
                return *this;
            }

            strcpy(this->text, other.text);
            this->type = other.type;

            return *this;
        }

        explicit Message(Type t) : text{}, type(t){};

        void appendText(const char *message)
        {
            strcat(this->text, message);
        };

        void _send(_SOCKET socket)
        {
            send(socket, (char *)this, sizeof(Message), 0);
        };

        char *getContent()
        {
            return this->text;
        }
    };

#ifdef _WIN32
    inline bool message_is_ready(string &input, string username)
    {
        char ch = 0;

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

        return false;
    }
#elif __linux__
    inline bool message_is_ready(string &input, string username)
    {
        char ch = 0;

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

        return false;
    }
#endif
}

#endif