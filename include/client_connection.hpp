#pragma once

class RedisServer;

class ClientConnection
{

private:
    int client_fd;
    RedisServer &server_;
public: 
    
    explicit ClientConnection(int fd, RedisServer& server);

    bool serveRequest();

    void closeConnection();

    bool isValid() const;

    int getFd() const;
    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};