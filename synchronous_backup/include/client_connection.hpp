#pragma once

#include <string>

class RedisServer;

class ClientConnection
{

private:
    int client_fd;
    bool is_blocked;
    RedisServer &server_;
public: 
    
    explicit ClientConnection(int fd, RedisServer& server);

    bool serveRequest();

    bool sendResponse(const std::string& response);

    void closeConnection();

    bool isValid() const;

    bool isBlocked() const;

    int getFd() const;
    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};