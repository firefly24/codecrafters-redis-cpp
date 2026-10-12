#pragma once

#include <thread>

#include "resp_encoder.hpp"


class ClientConnection
{

private:
    int client_fd;
    std::thread connector;
    RESPEncoder encoder;
public: 
    
    explicit ClientConnection(int fd);

    void serve();

    ~ClientConnection();

    // disable copy
    ClientConnection& operator=(const ClientConnection&) = delete;
    ClientConnection(const ClientConnection&) = delete;

    // disable move
    ClientConnection& operator=(ClientConnection&&) = delete;
    ClientConnection(ClientConnection&&) = delete;

};