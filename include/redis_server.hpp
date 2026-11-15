#pragma once

#include <stdio.h>
#include <chrono>
#include <cstdint>
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
#include <queue>
#include <utility>
//#include <asio.hpp>

#include "client_connection.hpp"
#include "message_types.hpp"

#define PORTNO 6379

enum class ExpiryTimeUnit
{
    MILLISECONDS,
    SECONDS
};

enum class KeyType;
using StreamEntry = std::vector<std::pair<std::string,std::string>>;
using Stream = std::vector<StreamEntry>;

using BlockedClientRef = std::weak_ptr<ClientConnection>;

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
        std::unordered_map<std::string, KeyType> key_item_types;
        std::unordered_map<std::string,KeyRecord> kv_store;
        std::unordered_map<std::string,std::vector<std::string>> list_store;
        std::unordered_map<std::string,Stream> streams_store;

        std::unordered_map<std::string,std::queue<BlockedClientRef>> waiting_clients;
       
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
        std::string getKeyType(const std::string& keyname);
        void setKeyType(const std::string& keyname, KeyType type);

        // for lists
        int list_push_front(const std::string& listname,std::vector<std::string>& element);
        int list_push_back(const std::string& listname,std::vector<std::string>& element);
        std::vector<std::string> list_pop_front(const std::string& listname,bool isblocking = false,int count =1);
        std::vector<std::string> getListRange(const std::string& list_name,int start,int stop);
        std::size_t getListSize(const std::string& listname);

        // for streams
        bool addStreamEntry(const std::string& stream_name,const std::string& entry_id,std::vector<std::string>& kv_pairs);

        void registerWaitingClient(const std::string& listname,BlockedClientRef client);
        //BlockedClientRef getNextBlockedRef(const std::string& listname);

        std::shared_ptr<ClientConnection> getNextBlockedRef(const std::string& listname);
    };

    int server_fd{-1};
    int port{PORTNO};
    bool is_polling{false};
    struct sockaddr_in server_addr{};
    struct sockaddr_in client_addr{};
    //std::vector<std::unique_ptr<ClientConnection>> clients;
    std::vector<std::shared_ptr<ClientConnection>> clients;
    SharedStore global_store;

    CommandResult handleSetKey(const std::vector<std::string>& cmd);
    CommandResult handleGetKey(const std::string& keyname);
    CommandResult handleGetType(const std::string& keyname);
    CommandResult handleListLength(const std::string& listname);
    CommandResult handleLeftRange(const std::string& listname,int start, int stop);
    CommandResult handleListPop(const std::string& listname, bool is_blocking, int array_count =0);
    CommandResult handleListPush(bool front,const std::vector<std::string>& cmd);

    CommandResult handleStreamAdd(const std::vector<std::string>& cmd);

public:

    explicit RedisServer();

    bool launchServer();

    bool connectNonBlockingClient();

    int generateValidFds(std::vector<pollfd>& poll_fds);

    void runEventLoop();

    CommandResult processCommand( const std::vector<std::string>& cmd);

    std::vector<std::string> processCommandComplex(const std::vector<std::string>& cmd);

    void requestWaiterRegistration(const std::string& listname,BlockedClientRef client);

    std::shared_ptr<ClientConnection> getNextWaitingClient(const std::string& listname);

    void wakeBlockedClient(std::string& listname);

    ~RedisServer();

    // Delete copy 
    RedisServer& operator=(const RedisServer&) = delete;
    RedisServer(const RedisServer&) = delete;
};