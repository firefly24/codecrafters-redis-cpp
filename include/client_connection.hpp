#pragma once

#include "message_types.hpp"
#include <string>
#include <memory>

class RedisServer;

class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{

private:
    int client_fd;
    RedisServer &server_;
    bool is_blocked;
public: 
    
    explicit ClientConnection(int fd, RedisServer& server);

    bool serveRequest();

    bool sendResponse(const std::string& response);

    std::string encodeResponse(CommandResult&);

    void closeConnection();

    bool isValid() const;

    bool isBlocked() const;

    void unblock();

    int getFd() const;
    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};