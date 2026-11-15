#include <cerrno>
#include <chrono>
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

        CommandResult response = server_.processCommand(cmd);

        if(!handleResponse(response))
            return false;

        if(cmd.size() && (cmd[0]=="LPUSH" || cmd[0]=="RPUSH"))
            server_.wakeBlockedClient(cmd[1]);
    }
    return true;
}

void ClientConnection::deferResponse(const CommandResult& response)
{
    //std::cerr << "serveRequest: setting client blocked" << std::endl;
    auto self = shared_from_this();
    server_.requestWaiterRegistration(response.result_array[0],self);
    
    if(response.result != "0")
    {
        double timeout_s = std::stod(response.result);
        int timeout_ms = static_cast<int>(timeout_s*1000.0);
        blockWithTimeout(timeout_ms);
    }
    else {
        blockWithTimeout(-1);
    }
}

bool ClientConnection::createAndSendResponse(const std::vector<std::string>& msg,MsgType type)
{
    CommandResult response;
    response.type = type;
    response.result_array = msg;

    return handleResponse(response);
}

bool ClientConnection::handleResponse(const CommandResult& response)
{
    if(response.type == MsgType::DEFER)
    {
        deferResponse(response);
        return true;
    }
    std::string reply = encodeResponse(response);
    return sendResponse(reply);
}

std::string ClientConnection::encodeResponse(const CommandResult& response)
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

void ClientConnection::blockWithTimeout(int timeout_ms)
{
    if(timeout_ms != -1)
    {
        block_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    }
    is_blocked = true;
}

void ClientConnection::unblock()
{
    is_blocked = false;
    block_deadline.reset();
}

int ClientConnection::getTimeoutMs()
{
    // No valid timeout exists
    if(!block_deadline)
        return -1;

    auto countdown_ms = *block_deadline - std::chrono::steady_clock::now();

    // timeout has already passed 
    if(countdown_ms <= std::chrono::steady_clock::duration::zero())
        return 0;
    
    // return milliseconds countdown to timeout
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(countdown_ms).count());
}

ClientConnection::~ClientConnection()
{
    closeConnection();
}
