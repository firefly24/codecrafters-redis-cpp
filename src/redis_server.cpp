#include <iostream>
#include <memory>
#include <stdexcept>
#include <poll.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <algorithm>

#include "redis_server.hpp"
#include "client_connection.hpp"

RedisServer::RedisServer()
{
    port = 6379;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        throw std::runtime_error("Failed to create server socket");
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server_fd);
        std::cerr << "setsockopt failed\n";
        throw std::runtime_error("setsockopt failed");
        
    }
}

bool RedisServer::launchServer()
{
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
  
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) 
    {
        std::cerr << "Failed to bind to port 6379\n";
        return false;
    }
  
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) 
    {
        std::cerr << "listen failed\n";
        return false;
    }
    std::cout << "Waiting for a client to connect...\n";

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";
    return true;
}


bool RedisServer::connectNonBlockingClient()
{
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr,&client_addr_len);
    
    if(client_fd < 0)
    {
        std::cerr << "accept failed!" << std::endl;
        return false;
    }

    // make connection non-blocking
    int flags = fcntl(client_fd,F_GETFL,0);
    if( (flags <0) || (fcntl(client_fd,F_SETFL,flags| O_NONBLOCK) < 0) )
    {
        std::cerr << "Failed to set client socket to non-blocking" << std::endl;
        close(client_fd);
        return false;
    }

    clients.push_back(std::make_unique<ClientConnection>(client_fd));

    std::cout << "Client connected\n";
    return true;
}

void RedisServer::runEventLoop()
{
    pollfd listener{};
    listener.fd = server_fd;
    listener.events = POLLIN;

    while(true)
    {
        std::vector<pollfd> poll_fds;
        poll_fds.push_back({server_fd,POLLIN, 0});

        // remove invalid/ closed client connections 
        std::erase_if(clients,[](const std::unique_ptr<ClientConnection>& conn){return !conn->isValid();});
        for (const auto& client_connection: clients)
        {
            poll_fds.push_back({client_connection->getFd(), POLLIN,0});
        }

        int ready_count = poll(poll_fds.data(), poll_fds.size(), -1);
        if(ready_count < 0)
        {
            std::cerr << "Poll failed" << std::endl;
            break;
        }

        if(poll_fds[0].revents & POLLIN)
        {
            if (!connectNonBlockingClient())
                std::cerr << "new client connection failed!" << std::endl;
            // connectNonBlockingClient already pushes the client_fd to clients[],
            // so it can be picked in poll_fds in next iteration
            // so I will not push it to poll_fds here   
        }

        if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL) )
        {
            std::cerr << "Listening socket reported error!" << std::endl;
            break;
        }

        for(std::size_t i=1; i<poll_fds.size(); i++)
        {
            if(poll_fds[i].revents & POLLIN)
            {
                if(!clients[i-1]->serveRequest())
                {
                    clients[i-1]->closeConnection();
                }
            }
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL) )
            {
                // What to do where when client poll fails ? 
                clients[i-1]->closeConnection();
            }
        }
    }
}

RedisServer::~RedisServer()
{
    if(server_fd >= 0)
        close(server_fd);
}