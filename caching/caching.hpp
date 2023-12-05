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
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <shlobj.h>
#define _MAX_PATH MAX_PATH
#define _STR_COPY wcscpy
#define _STR_LEN wcslen
#define _STR_CAT wcscat
typedef wchar_t _PATH_CHAR;

#else

#include <limits.h>
#define _MAX_PATH PATH_MAX
#define _STR_COPY strcpy
#define _STR_LEN strlen
#define _STR_CAT strcat
typedef char _PATH_CHAR;
#endif

#define PATH_NOT_FOUND 1
#define FILE_ALREADY_EXISTS 2
#define FILE_NOT_ALREADY_EXISTS 3
#define FILE_EXIST__NEW_MESSAGE 4

namespace clca
{
    _PATH_CHAR path[_MAX_PATH];
    _PATH_CHAR *path_ref;
    fstream chatcache;
    int dirtyflag;

    int load_chat(string &chatbuffer, string &newmessages_for_sending, string &newmessages, const char *foldername, const char *filename, string &myname)
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
        #else
        path_ref = getenv("XDG_DATA_HOME");

        if(path_ref== NULL || path_ref[0]=='\0'){ //XDG_DATA_HOME not set
            path_ref = getenv("HOME"); 

            if(path_ref== NULL || path_ref[0]=='\0'){ //HOME not set
                path_ref="/tmp";
            }
            else{
                strcat(path_ref,"/.local/share");
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
            #else
                _STR_COPY(wc,foldername);
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
            #else
            _STR_COPY(wc,filename);
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
                    if ((line == "\033[38;2;255;0;0mThe client is disconnected\033[0m") || (line == "\033[38;2;255;0;0mThe server is disconnected\033[0m"))
                    {
                        dirtyflag = FILE_EXIST__NEW_MESSAGE;

                        continue;
                    }

                    if (dirtyflag == FILE_EXIST__NEW_MESSAGE)
                    {
                        newmessages += (line + "\n");
                        newmessages_for_sending += (myname + ":" + line.substr(4) + "\n");
                    }
                    else
                    {
                        chatbuffer += (line + "\n");
                    }
                }

                if (newmessages == "")
                {
                    dirtyflag = FILE_ALREADY_EXISTS;
                }
            }
            chatcache.close();
            chatcache.clear();
        #ifdef _WIN32
        }
        #endif

        return dirtyflag;
    }

    int save_chat(string &chatbuffer, string username)
    {
        chatcache.open(path, fstream::in | fstream::out | fstream::trunc); // TODO created another file every time(it's not good for long chat)
        chatcache << username + "\n";
        chatcache << chatbuffer;
        chatcache.close();
        chatcache.clear();

        return 1;
    }
}

namespace srca
{
    // TODO
}

#endif