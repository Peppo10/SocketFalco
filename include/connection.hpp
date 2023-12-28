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

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#ifdef _WIN32
#include <ws2tcpip.h>
#include "service.hpp"
#include <thread>
#define FAILED_TO_CONNECT 404

#define _SOCKET_INV INVALID_SOCKET
#define _SOCKET_ERR SOCKET_ERROR
#define _CLEAR "cls"
typedef int _ADDR_LEN;
#elif __linux__
#include <arpa/inet.h>
#include <sys/types.h>

#define _SOCKET_INV -1
#define _SOCKET_ERR -1
#define _CLEAR "clear"
typedef u_int _ADDR_LEN;
#endif

#define PORT 30000

using namespace std;

//CLIENT
_SOCKET local_socket;
sockaddr_in remote_socket_addr; // socket address romote side

//SERVER
_SOCKET acceptedSocket;
sockaddr_in service;  // socket address for listening server socket
_ADDR_LEN client_addr_len = sizeof(sockaddr_in);

#ifdef _WIN32
WSADATA wsadata;
WORD versionRequested = MAKEWORD(2, 2);
#endif

condition_variable cv;
mutex m1;

clca::Chat chat;
string input;
string username;
basic_string<_PATH_CHAR> remote_uuid;
string uuid;

int remote_connect = srv::DISCONNECT;
int file_flag;
bool notified = false;

#endif