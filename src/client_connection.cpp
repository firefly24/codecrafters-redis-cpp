#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

#include "redis_server.hpp"
#include "resp_encoder.hpp"
#include "resp_parser.hpp"

ClientConnection::ClientConnection(int fd, RedisServer& server): client_fd(fd), server_(server)
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
        // TODO: incomplete send/receive not supported yet
        std::vector<std::string> cmd = RESPParser::parse(std::string{buffer,(std::size_t)received});

        //for(const auto& word : cmd)
        //    std::cout << word << "*" << std::endl;

        std::string result ="";
        MsgType encoding_type = server_.processCommand(cmd,result);

        std::string response = RESPEncoder::encodeMsg(result, encoding_type);

        ssize_t sent = send(client_fd,response.data(), response.size(), 0);
        if(sent < 0)
        {
            std::cerr << "Send failed" << std::endl;
        }
    }
    return true;
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
