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

#include "../include/client.hpp"
#include "../include/server.hpp"
#include <string.h>
#include <stdio.h>
#include <filesystem>
#include <fstream>

int main(int argc, char *argv[])
{

    if (argc == 2)
    {
        if ((strcmp(argv[1], "-listen") == 0) || (strcmp(argv[1], "-l") == 0))
        {
            return start_server(false);
        }

        if ((strcmp(argv[1], "-list") == 0) || (strcmp(argv[1], "-ls") == 0))
        {
            return clca::list_chat();
        }

        if ((strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "-h") == 0))
        {
            return clca::show_help();
        }

        cout << "Invalid arguments";
        return EXIT_FAILURE;
    }

    if (argc == 3)
    {
        if ((strcmp(argv[1], "-listen") == 0) || (strcmp(argv[1], "-l") == 0))
        {
            if(strcmp(argv[2],"-t") == 0)
                return start_server(true);
            else
                cout << argv[2] << " is not a -listen option";

            return EXIT_FAILURE;
        }

        if ((strcmp(argv[1], "-connect") == 0) || (strcmp(argv[1], "-c") == 0))
        {
            return start_client(argc - 2, &argv[2], false);
        }

        if ((strcmp(argv[1], "-editName") == 0) || (strcmp(argv[1], "-eN") == 0))
        {
            return clca::update_name(argv[2]);
        }

        if((strcmp(argv[1],"-editChat") == 0) || (strcmp(argv[1],"-eC") == 0))
        {
            return add_new_messages(argv[2]);
        }

        cout << "Invalid arguments";
        return EXIT_FAILURE;
    }

    if (argc == 4)
    {
        if ((strcmp(argv[1], "-connect") == 0) || (strcmp(argv[1], "-c") == 0))
        {
            if(strcmp(argv[2],"-t") == 0)
                return start_client(argc - 3, &argv[3], true);
            else
                cout << argv[2] << " is not a -connect option";
            
            return EXIT_FAILURE;
        }

        cout << "Invalid arguments";
        return EXIT_FAILURE;
    }
}
