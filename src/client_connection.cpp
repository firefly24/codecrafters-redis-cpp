#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

ClientConnection::ClientConnection(int fd): client_fd(fd)
{

}

int ClientConnection::getFd() const
{
    return client_fd;
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

/* Returns false only when client is no longer servicable, 
    like when client closes connection
    return true otherwise
*/
bool ClientConnection::serveRequest()
{
    bool is_servicable = (client_fd >= 0);

    ssize_t received = 0;

    if(is_servicable)
    {
        char buffer[1024];
        received = recv(client_fd,buffer,sizeof(buffer),0);
    
        if(received < 0 )
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                is_servicable = true;
            }
            else 
            {
                std::cerr << "Receive failed!" << std::endl;
                is_servicable = false;
            }
        }
        else if( received ==0 )
        {
            // client disconnected the connection
            //return false;
            is_servicable = false;
        }
        else 
        {
            std::string msg = encoder.encodeMsg("PONG");

            ssize_t sent = send(client_fd,msg.data(), msg.size(), 0);
            if(sent < 0)
            {
                std::cerr << "Send failed" << std::endl;
                //return true;
            }
        }
    }
    return is_servicable;
}

void ClientConnection::closeConnection()
{
    if(client_fd>=0)
        close(client_fd);
    client_fd = -1;
}

bool ClientConnection::isValid()
{
    return (client_fd != -1);
}

ClientConnection::~ClientConnection()
{
    closeConnection();
}
