#pragma once

#include <vector>

class ClientConnection
{

private:
    int client_fd;
public: 
    
    explicit ClientConnection(int fd);

    void serve();

    bool serveRequest();

    std::string processCommand(const std::vector<std::string>& cmd);

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