#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

ClientConnection::ClientConnection(int fd): client_fd(fd)
{

}

void ClientConnection::serve()
{
    if(client_fd < 0 )
        return;

    ssize_t received = 0;
    while(true)
    {
        char buffer[1024];
        received = recv(client_fd,buffer,sizeof(buffer),0);
    
        if(received < 0 )
        {
            std::cerr << "Receive failed!" << std::endl;
            break;
        }
        else if( received ==0 )
        {
            // client disconnected the connection
            break;
        }

        std::string msg = encoder.encodeMsg("PONG");

        ssize_t sent = send(client_fd,msg.data(), msg.size(), 0);
        if(sent < 0)
        {
            std::cerr << "Send failed" << std::endl;
            break;
        }
    }
}

ClientConnection::~ClientConnection()
{
    if(client_fd >= 0)
        close(client_fd);
}
