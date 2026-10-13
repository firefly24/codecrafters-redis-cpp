#pragma once

#include <vector>
#include "resp_encoder.hpp"
//#include "resp_parser.hpp"


class ClientConnection
{

private:
    int client_fd;
    RESPEncoder encoder;
public: 
    
    explicit ClientConnection(int fd);

    void serve();

    bool serveRequest();

    std::string commandExecutor(std::vector<std::string> cmd);

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