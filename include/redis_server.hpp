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
#include <unordered_map>

#include "client_connection.hpp"
#include "message_types.hpp"

#define PORTNO

class RedisServer
{
private:

    class SharedStore
    {
    private:
        std::unordered_map<std::string,std::string> kv_store;
    public:
        SharedStore() = default;
        // delete copy
        SharedStore& operator=(const SharedStore&) = delete;
        SharedStore(const SharedStore&) = delete;

        //delete move
        SharedStore& operator=(SharedStore&&) = delete;
        SharedStore(SharedStore&&) = delete;

        bool getValue(const std::string& key, std::string& value);
        void setValue(const std::string&key,const std::string& value);
    };

    int server_fd{-1};
    int port{PORTNO};
    struct sockaddr_in server_addr{};
    struct sockaddr_in client_addr{};
    std::vector<std::unique_ptr<ClientConnection>> clients;
    SharedStore global_store;

public:

    explicit RedisServer();

    bool launchServer();

    bool connectNonBlockingClient();

    void generateValidFds(std::vector<pollfd>& poll_fds);

    void runEventLoop();

    MsgType processCommand(const std::vector<std::string>& cmd, std::string& response);

    ~RedisServer();

    // Delete copy 
    RedisServer& operator=(const RedisServer&) = delete;
    RedisServer(const RedisServer&) = delete;
};