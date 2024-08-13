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