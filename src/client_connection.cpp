#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

#include "message_types.hpp"
#include "redis_server.hpp"
#include "resp_encoder.hpp"
#include "resp_parser.hpp"

ClientConnection::ClientConnection(int fd, RedisServer& server): client_fd(fd), 
                                                                 server_(server),
                                                                 is_blocked(false)
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

        std::string reply ="";
        //std::string response;
        //MsgType encoding_type = server_.processCommand(cmd,result);
        CommandResult response = server_.processCommand(cmd);

        if(response.type == MsgType::DEFER)
        {
            //std::cerr << "serveRequest: setting client blocked" << std::endl;
            is_blocked = true;
            auto self = shared_from_this();
            server_.requestWaiterRegistration(response.result,self);
            return true;
        }
        
        reply = encodeResponse(response);

        if(!sendResponse(reply))
            return false;

        if(cmd.size() && (cmd[0]=="LPUSH" || cmd[0]=="RPUSH"))
            server_.wakeBlockedClient(cmd[1]);
    }
    return true;
}

std::string ClientConnection::encodeResponse(CommandResult& response)
{
    if(response.type == MsgType::ARRAY)
    {
        return RESPEncoder::encodeMsgArray(response.result_array);
    }
    else 
    {
        return RESPEncoder::encodeMsg(response.result, response.type);
    }
}

bool ClientConnection::sendResponse(const std::string& response)
{
    ssize_t sent = send(client_fd,response.data(), response.size(), 0);
    if(sent < 0)
    {
        std::cerr << "Send failed" << std::endl;
        //return false;
        // right now we don't know what to do for send failures, retry or close connection
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

bool ClientConnection::isBlocked() const
{
    return is_blocked;
}

void ClientConnection::unblock()
{
    is_blocked = false;
}

ClientConnection::~ClientConnection()
{
    closeConnection();
}
