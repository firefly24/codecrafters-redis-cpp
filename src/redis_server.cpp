#include <iostream>
#include <stdexcept>

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

bool RedisServer::acceptClientConnection()
{
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr,&client_addr_len);
    
    if(client_fd < 0)
    {
        std::cerr << "accept failed!" << std::endl;
        return false;
    }

    serveClient(client_fd);

    std::cout << "Client connected\n";
    return true;
}

void RedisServer::serveClient(int client_fd)
{
    if(client_fd < 0 )
        return;

    server_threads.emplace_back([this,client_fd](){
                                    ClientConnection connection(client_fd);
                                    connection.serve();
                                });

}

RedisServer::~RedisServer()
{
    for(auto& connection: server_threads)
    {
        if (connection.joinable())
            connection.join();
    }
    if(server_fd >= 0)
        close(server_fd);
}