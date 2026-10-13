#pragma once

#include "resp_encoder.hpp"


class ClientConnection
{

private:
    int client_fd;
    RESPEncoder encoder;
public: 
    
    explicit ClientConnection(int fd);

    void serve();

    bool serveRequest();

    void closeConnection();

    bool isValid();

    int getFd() const;
    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};