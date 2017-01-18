#include <iostream>
#include <cstdlib>
#include <cstring>
#include "client.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


void Client::send_command(char type, int ttl, string key, string value) {
    // std::cout << "DEBUG: type=" << int(type) << "; ttl=" << ttl << "; key='" << key << "'; value='" << value << "'\n";
    // Get buf - char pointer to buffer, place here type, ttl, key, value
    char *buf;
    // Len + Command
    int len = sizeof(int) + sizeof(char); // + key.size() + value.size() + sizeof(char) * 2;
    switch(type) {
        case SET:
        {
            len += key.size() + 1 + value.size() + 1 + sizeof(ttl);
            buf = (char *)calloc(len * sizeof(char), sizeof(char));

            char *ptr = buf;
            memcpy(ptr, &len, sizeof(len)); ptr += sizeof(len);
            memcpy(ptr, &type, sizeof(type)); ptr += sizeof(type);

            memcpy(ptr, &ttl, sizeof(ttl)); ptr += sizeof(ttl);
            memcpy(ptr, key.c_str(), key.size()); ptr += key.size() + 1;
            memcpy(ptr, value.c_str(), value.size());

            break;
        }
        case GET: case DELETE:
        {
            len += key.size() + 1;
            buf = (char *)calloc(len * sizeof(char), sizeof(char));
            char *ptr = buf;
            memcpy(ptr, &len, sizeof(len)); ptr += sizeof(len);
            memcpy(ptr, &type, sizeof(type)); ptr += sizeof(type);

            memcpy(ptr, key.c_str(), key.size());

            break;
        }
        default:
        {
            buf = (char *)calloc(len * sizeof(char), sizeof(char));
            memcpy(buf, &len, sizeof(len));
            sprintf(buf + sizeof(len), "%c", type);
            break;
        }
    }

/*    fprintf(stderr, "len = %d\n", len);
    for (uint i = 0; i < len; i++) {
        fprintf(stderr, "%u ", buf[i]);
    }

    fprintf(stderr, "\n");
*/
    send(this->sockfd, buf, len, 0);
}

Client::Client() {
    // std::cout << "DEBUG: " << "Entered constructor" << std::endl;
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8090);

    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    socklen_t len = sizeof(server);

    int r = connect(sockfd, (struct sockaddr *) &server, len);
    fprintf(stderr, "%s\n", strerror(errno));
}

std::string Client::receive() {
    uint32_t len;
    int r = recv(sockfd, &len, sizeof(len), 0);
    if (r == -1) {
        fprintf(stderr, "Failed: %s\n", strerror(errno));
        return "";
    }

    // fprintf(stderr, "Got: %d\n", len);

    char buf[len + 1]; char *ptr = buf;
    r = 0; int left = len;

    while (left) {
        r = recv(sockfd, ptr, left, 0);
        left -= r;
        ptr += r;
    }

    buf[len] = 0;
    // printf("Body: %s\n", buf);

    return std::string(buf);
}

static std::string com2string[] = {
    "connect",
    "disconnect",
    "get",
    "set",
    "delete"
};

int test(Client & client, char command, uint32_t ttl, std::string key, std::string value, int timeout, std::string result) {
    static int testnum = 0; testnum += 1;

    std::cout << testnum << ": " << com2string[command] << " (" << ttl << ") " << key << " => " << value << "\n";

    sleep(timeout);
    client.send_command(command, ttl, key, value);
    std::string response = client.receive();

    if (response == result) {
        std::cout << testnum << ": OK!\n";
        return 0;
    } else {
        std::cerr << testnum << ": Failed!\n";
        std::cerr << "Expected: " << result << "\n";
        std::cerr << "Got     : " << response << "\n";
        return 1;
    }
}

int base() {
    Client cl = Client();

    int failed = 0;
    failed += test(cl, SET, 1, "key", "value", 0, "ok");

    
    failed += test(cl, SET, 100, "ololo", "val1", 0, "ok");
    failed += test(cl, GET, 0  , "ololo", ""    , 0, "val1");

    failed += test(cl, SET, 100, "ololo", "val2", 0, "ok");
    failed += test(cl, GET, 0  , "ololo", ""    , 0, "val2");

    failed += test(cl, SET, 100, "ololo", "value3", 0, "ok");
    failed += test(cl, GET, 0  , "ololo", ""      , 0, "value3");

    failed += test(cl, SET, 100, "ololo", "v4", 0, "ok");
    failed += test(cl, GET, 0  , "ololo", ""  , 0, "v4");

    return failed;
}

int not_found() {
    Client cl = Client();

    int failed = 0;
    failed += test(cl, GET, 1, "kek", "", 0, "not_found");

    return failed;
}

int expired() {
    Client cl = Client();

    int failed = 0;
    failed += test(cl, SET, 1, "fuck", "fuck", 0, "ok");
    failed += test(cl, GET, 1, "fuck", "", 2, "not_found");

    return failed;
}

int remove() {
    Client cl = Client();

    int failed = 0;
    failed += test(cl, SET   , 100, "testdel", "blabla", 0, "ok");
    failed += test(cl, DELETE, 0  , "testdel", ""      , 0, "ok");
    failed += test(cl, GET   , 0  , "testdel", ""      , 0, "not_found");

    return failed;
}

int remove2() {
    Client cl = Client();

    int failed = 0;
    failed += test(cl, DELETE, 0  , "not_found", "", 0, "ok");

    return failed;    
}

int bigtest() {
    Client cl = Client();
    std::string msg(10 * 1000 + 1, 'a');

    int failed = 0;
    failed += test(cl, SET, 0  , "big", msg, 0, "failed");

}

int main(int argc, char const *argv[])
{
    int failed = 0;

    failed += base();
    failed += expired();
    failed += remove();
    failed += remove2();
    failed += bigtest();

    std::cout << "Done testing with: " << failed << " errors\n";
    return 0;
}