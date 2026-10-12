#pragma once

#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <sys/_types/_in_port_t.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <thread>

#define PORTNO

class RedisServer
{
private:

    //RESPEncoder encoder;
    int server_fd{-1};
    int port{PORTNO};
    struct sockaddr_in server_addr{};
    struct sockaddr_in client_addr{};
    //int clientfd{-1};
    //std::vector<ClientConnection> client_conns;
    std::vector<std::thread> server_threads;

public:

    explicit RedisServer();

    bool launchServer();

    bool acceptClientConnection();

    void serveClient(int client_fd);

    ~RedisServer();

    // Delete copy 
    RedisServer& operator=(const RedisServer&) = delete;
    RedisServer(const RedisServer&) = delete;
};