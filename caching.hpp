#ifndef CACHING_H
#define CACHING_H
#include <filesystem>
#include <fstream>
#include <shlobj.h>

#define PATH_NOT_FOUND 1
#define FILE_ALREADY_EXISTS 2
#define FILE_NOT_ALREADY_EXISTS 3
#define FILE_EXIST__NEW_MESSAGE 4

namespace clca
{
    wchar_t path[MAX_PATH];
    wchar_t *path_ref;
    fstream chatcache;
    int dirtyflag;

    int load_chat(string &chatbuffer, string &newmessages_for_sending, string &newmessages, const char *foldername, const char *filename, string &myname)
    {
        if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &path_ref)))
        { // get /user/appdata/localLow folder
            cout << "Directory not found!";
            system("pause");
            return PATH_NOT_FOUND;
        }
        else
        {
            wcscpy(path, path_ref);
            for (size_t i = 0; i < wcslen(path); i++)
            { // switch \ to / because C:\..\..\.. is invalis path
                if (path[i] == '\\')
                {
                    path[i] = '/';
                }
            }

            wchar_t *wc = new wchar_t[strlen(foldername) + 1];
            mbstowcs(wc, foldername, strlen(foldername) + 1);
            wcscat(path, wc);

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

            wc = new wchar_t[strlen(filename) + 1];
            mbstowcs(wc, filename, strlen(filename) + 1);
            wcscat(path, wc);

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
        }

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