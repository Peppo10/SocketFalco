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
            return start_server();
        }

        if ((strcmp(argv[1], "-list") == 0) || (strcmp(argv[1], "-ls") == 0))
        {
            auto path = clca::getCacheDir();

            std::cout << "\033[38;2;255;255;0mList of your chats in cache:\033[0m\n";

            size_t c = 1;
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                fstream f(entry.path().c_str(), fstream::in | fstream::out);

                string username;
                f >> username;

                std::cout << c++ << ">" << entry.path().filename() << "(" << username << ")" << std::endl;
            }

            return EXIT_SUCCESS;
        }

        cout << "Invalid arguments";
        return EXIT_FAILURE;
    }

    if (argc == 3)
    {
        if ((strcmp(argv[1], "-connect") == 0) || (strcmp(argv[1], "-c") == 0))
        {
            return start_client(argc - 2, &argv[2]);
        }

        if ((strcmp(argv[1], "-editName") == 0) || (strcmp(argv[1], "-eN") == 0))
        {
            return clca::update_name(argv[2]);
        }

        if((strcmp(argv[1],"-editChat") == 0) || (strcmp(argv[1],"-eC") == 0)){
            return add_new_messages(argv[2]);
        }

        cout << "Invalid arguments";
        return EXIT_FAILURE;
    }
}