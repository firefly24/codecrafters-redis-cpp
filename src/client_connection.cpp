#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

#include "resp_encoder.hpp"
#include "resp_parser.hpp"

ClientConnection::ClientConnection(int fd): client_fd(fd)
{

}

int ClientConnection::getFd() const
{
    return client_fd;
}

/* Returns false only when client is no longer servicable, 
    like when client closes connection
    return true otherwise
*/
bool ClientConnection::serveRequest()
{
    if(client_fd < 0)
        return false;

    bool is_servicable = true;

    char buffer[1024];
    ssize_t received = recv(client_fd,buffer,sizeof(buffer),0);

    if( received ==0 )
        return false;

    if(received < 0 )
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return true;

        std::cerr << "Receive failed!" << std::endl;
        return false;
    }
    else 
    {
        std::vector<std::string> cmd = RESPParser::parse(std::string{buffer,(std::size_t)received});

        std::string result = processCommand(cmd);
        std::string response = RESPEncoder::encodeMsg(result);

        ssize_t sent = send(client_fd,response.data(), response.size(), 0);
        if(sent < 0)
        {
            std::cerr << "Send failed" << std::endl;
        }
    }
    return is_servicable;
}

std::string ClientConnection::processCommand(const std::vector<std::string>& cmd)
{
    if(cmd.empty())
        return "";

    if(cmd[0] == "PING")
        return "PONG";

    if(cmd[0] == "ECHO")
    {
        if(cmd.size() >1)
            return cmd[1];
    }
    return "";
}

void ClientConnection::closeConnection()
{
    if(client_fd>=0)
        close(client_fd);
    client_fd = -1;
}

bool ClientConnection::isValid() const
{
    return (client_fd != -1);
}

ClientConnection::~ClientConnection()
{
    closeConnection();
}
