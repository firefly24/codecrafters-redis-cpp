#pragma once

#include "message_types.hpp"
#include <optional>
#include <ratio>
#include <string>
#include <memory>
#include <chrono>
#include <optional>

class RedisServer;

class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{

private:
    int client_fd;
    RedisServer &server_;
    bool is_blocked;
    std::optional<std::chrono::steady_clock::time_point> block_deadline;
public: 
    
    explicit ClientConnection(int fd, RedisServer& server);

    bool serveRequest();
    bool handleResponse(const CommandResult& response);

    void deferResponse(const CommandResult& response);
    std::string encodeResponse(const CommandResult&);
    bool sendResponse(const std::string& response);

    void closeConnection();

    bool isValid() const;

    bool isBlocked() const;

    void unblock();

    void blockWithTimeout(int timeout_ms = -1);

    int getTimeoutMs();

    int getFd() const;
    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};