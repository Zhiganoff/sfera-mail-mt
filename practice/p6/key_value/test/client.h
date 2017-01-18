// #pragma once

#include <sys/socket.h>
#include <string>

using std::string;

#define BYTES_PER_INT 4

enum command_type {
    CONNECT = 0x0,
    DISCONNECT,
    GET,
    SET,
    DELETE
};

int get_int(char* p);

class Client {
    int sockfd;
public:
    void send_command(char type, int ttl, string key, string value);
    std::string receive();
    Client();
};