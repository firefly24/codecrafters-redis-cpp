#pragma once
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <sys/poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <memory>

#include "client_connection.hpp"

#define PORTNO

class RedisServer
{
private:

    int server_fd{-1};
    int port{PORTNO};
    struct sockaddr_in server_addr{};
    struct sockaddr_in client_addr{};
    std::vector<std::unique_ptr<ClientConnection>> clients;

public:

    explicit RedisServer();

    bool launchServer();

    bool connectNonBlockingClient();

    void generateValidFds(std::vector<pollfd>& poll_fds);

    void runEventLoop();

    ~RedisServer();

    // Delete copy 
    RedisServer& operator=(const RedisServer&) = delete;
    RedisServer(const RedisServer&) = delete;
};