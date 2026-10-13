#include <cerrno>
#include <string_view>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "client_connection.hpp"

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
    bool is_servicable = (client_fd >= 0);

    ssize_t received = 0;

    if(is_servicable)
    {
        char buffer[1024];
        received = recv(client_fd,buffer,sizeof(buffer)-1,0);
    
        if( received ==0 )
        {
            is_servicable = false;
        }
        else if(received < 0 )
        {
            is_servicable = (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);

            if(!is_servicable)
                std::cerr << "Receive failed!" << std::endl;
        }
        else 
        {
            std::vector<std::string> cmd = RESPParser::parse(std::string{buffer,(std::size_t)received});
            //for(const auto& word: cmd)
            //    std::cout << word << "* " ;
            //std::cout << buffer << std::endl;

            std::string response,result;
            if(!cmd.empty() && cmd[0] == "PING")
                response = "PONG";
            else
            {
                result = commandExecutor(cmd);
                response = encoder.encodeMsg(result);
            }

            ssize_t sent = send(client_fd,response.data(), response.size(), 0);
            if(sent < 0)
            {
                std::cerr << "Send failed" << std::endl;
            }
        }
    }
    return is_servicable;
}

std::string ClientConnection::commandExecutor(std::vector<std::string> cmd)
{
    if(cmd.empty())
        return "";

    if(cmd[0] == "PING")
        return "PONG";

    if(cmd[0] == "ECHO")
    {
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

bool ClientConnection::isValid()
{
    return (client_fd != -1);
}

ClientConnection::~ClientConnection()
{
    closeConnection();
}
