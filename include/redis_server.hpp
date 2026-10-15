#pragma once

#include <chrono>
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

#define PORTNO 6379

enum class ExpiryTimeUnit
{
    MILLISECONDS,
    SECONDS
};

class RedisServer
{
private:

    class SharedStore
    {
    private:

        struct KeyRecord
        {
            std::string value;
            std::optional<std::chrono::steady_clock::time_point> expiry;
        };
        std::unordered_map<std::string,KeyRecord> kv_store;
        std::unordered_map<std::string,std::vector<std::string>> list_store;
    public:
        SharedStore() = default;
        // delete copy
        SharedStore& operator=(const SharedStore&) = delete;
        SharedStore(const SharedStore&) = delete;

        //delete move
        SharedStore& operator=(SharedStore&&) = delete;
        SharedStore(SharedStore&&) = delete;

        // For KV store
        bool getValue(const std::string& key, std::string& value);
        void setValue(const std::string&key,const std::string& value, int expiry = -1, ExpiryTimeUnit unit = ExpiryTimeUnit::MILLISECONDS);

        // for lists
        int list_push_front(const std::string& list,const std::vector<std::string>& element);
        int list_push_back(const std::string& list,const std::vector<std::string>& element);
        std::vector<std::string> getListRange(const std::string& list_name,int start,int stop);
        std::size_t getListSize(const std::string& listname);

    };

    int server_fd{-1};
    int port{PORTNO};
    bool is_polling{false};
    struct sockaddr_in server_addr{};
    struct sockaddr_in client_addr{};
    std::vector<std::unique_ptr<ClientConnection>> clients;
    SharedStore global_store;

    std::vector<std::string> handleLeftRange(const std::vector<std::string>& cmd);
    MsgType handleSetKey(const std::vector<std::string>& cmd, std::string& response);
    MsgType handleListPush(const std::vector<std::string>& cmd,std::string& response,bool front);

public:

    explicit RedisServer();

    bool launchServer();

    bool connectNonBlockingClient();

    void generateValidFds(std::vector<pollfd>& poll_fds);

    void runEventLoop();

    MsgType processCommand(const std::vector<std::string>& cmd, std::string& response);

    std::vector<std::string> processCommandComplex(const std::vector<std::string>& cmd);

    ~RedisServer();

    // Delete copy 
    RedisServer& operator=(const RedisServer&) = delete;
    RedisServer(const RedisServer&) = delete;
};