#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <poll.h>
#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <vector>

#include "redis_server.hpp"
#include "client_connection.hpp"
#include "message_types.hpp"

RedisServer::RedisServer()
{
    port = PORTNO;
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
    
    clients.push_back(std::make_unique<ClientConnection>(client_fd, *this));

    std::cout << "Client connected\n";
    return true;
}

void RedisServer::generateValidFds(std::vector<pollfd>& poll_fds)
{
    poll_fds.clear();   
    poll_fds.push_back({server_fd,POLLIN, 0});

    // remove invalid/ closed client connections 
    std::erase_if(clients,[](const std::unique_ptr<ClientConnection>& conn){return !conn->isValid();});
    for (const auto& client_connection: clients)
    {
        short events = (client_connection->isBlocked()?0:POLLIN);
        poll_fds.push_back({client_connection->getFd(),events,0});
    }
}

void RedisServer::runEventLoop()
{
    if(is_polling)
    {
        std::cerr << "server event loop already running" << std::endl;
        return;
    }

    // right now the server is single threaded, so this should be safe
    // reconsider changing such status variables to atomic/inside lock, when moving to multithreaded design
    is_polling = true; 
    while(true)
    {
        std::vector<pollfd> poll_fds;
        generateValidFds(poll_fds);

        int ready_count = poll(poll_fds.data(), poll_fds.size(), -1);
        if(ready_count < 0)
        {
            // TODO: handle interrupted by a signal case in future
            if ( errno == EINTR )
                continue;

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
            std::cerr << "POLLIN: blocked=" << clients[i-1]->isBlocked() << std::endl;
            if( (!clients[i-1]->isBlocked()) && (poll_fds[i].revents & POLLIN) )
            {
                if(!clients[i-1]->serveRequest())
                    clients[i-1]->closeConnection();
            }
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL) ) 
                clients[i-1]->closeConnection();
        }
    }
}


MsgType RedisServer::processCommand(const std::vector<std::string>& cmd,std::string& response)
{
    if(cmd.empty())
    {
        response = "";
        return MsgType::EMPTY;
    }

    // std::cout << cmd[0] << std::endl;

    if(cmd[0] == "PING")
    {
        response = "PONG";
        return MsgType::SIMPLE;
    }
    if(cmd[0] == "ECHO")
    {
        if(cmd.size() >1)
        {
            response = cmd[1];
            return MsgType::BULK;
        }
    }
    if(cmd[0] == "GET")
    {
        if(cmd.size() > 1)
        {
            if(global_store.getValue(cmd[1],response))
                return MsgType::BULK;
            return MsgType::EMPTY;
        }
    }
    if(cmd[0] == "RPUSH")
    {
        return handleListPush(cmd,response,false);
    }
    if(cmd[0] == "LPUSH")
    {
        return handleListPush(cmd, response, true);
    }

    if(cmd[0] == "SET")
    {
        return handleSetKey(cmd,response);
    }
    if(cmd[0] == "LRANGE")
    {
        return MsgType::ARRAY;
    }
    if(cmd[0] == "LLEN")
    {
        std::size_t list_size = 0;

        if (cmd.size() >= 2)
            list_size = global_store.getListSize(cmd[1]);

        response = std::to_string(list_size);
        return MsgType::INTEGER;
    }
    if(cmd[0] == "LPOP")
    { 
        if(cmd.size() == 3)
            return MsgType::ARRAY;
        if(cmd.size() == 2)
        {
            std::vector<std::string> reply = handleListPop(cmd);
            if(!reply.empty())
            {
                response = std::move(reply[0]);
                return MsgType::BULK;
            }
        }
        return MsgType::EMPTY;
    }
    if(cmd[0] == "BLPOP")
    {
        // Since the server execution is single threaded for now,
        // this operation just returns if an element is available for immediate pop without actually popping
        // The actual pop happens in the second call - ProcessCommandComplex, 
        // and no other client modifies the list inbetween for single threaded process
        if(cmd.size() == 3)
        {
            int list_size = global_store.getListSize(cmd[1]);
            if(list_size)
                return MsgType::ARRAY;
            else
            {
                return MsgType::DEFER;
            }
        }
    }

    response = "";
    return MsgType::INVALID;
}

std::vector<std::string> RedisServer::processCommandComplex(const std::vector<std::string>& cmd)
{
    if(cmd.empty())
        return {};

    if( cmd[0] == "LRANGE" )
    {
        return handleLeftRange(cmd);
    }
    if(cmd[0] == "LPOP")
        return handleListPop(cmd);

    if( (cmd[0] == "BLPOP") && (cmd.size() == 3))
    {
        return handleListPop(cmd);
        //return reply;
    }
    return {};
}

MsgType RedisServer::handleSetKey(const std::vector<std::string>& cmd, std::string& response)
{
    if(cmd.size() >=3 && cmd[0] == "SET")
    {
        if(cmd.size() == 3)
        {
            global_store.setValue(cmd[1],cmd[2]); 
            response = "OK";
            return MsgType::SIMPLE;
        }
        else if(cmd.size() == 5)
        {
            int duration  = std::stoi(cmd[4]);

            if(cmd[3] == "EX")
            {
                global_store.setValue(cmd[1],cmd[2],duration,ExpiryTimeUnit::SECONDS);
            }
            else if(cmd[3] == "PX")
            {
                global_store.setValue(cmd[1], cmd[2], duration,ExpiryTimeUnit::MILLISECONDS);
            }
            else {
                response = "";
                return MsgType::INVALID;
            }
            response = "OK";
            return MsgType::SIMPLE;
        }
    }
    response = "";
    return MsgType::INVALID;
}

MsgType RedisServer::handleListPush(const std::vector<std::string>& cmd,std::string& response,bool front)
{
    int list_size = -1;
    if ( (cmd.size() >= 3) )
    {
        if( (cmd[0] == "RPUSH") && (!front) )
            list_size = global_store.list_push_back(cmd[1], cmd);
        if( (cmd[0]=="LPUSH") && front )
            list_size = global_store.list_push_front(cmd[1], cmd);
    }
    if(list_size == -1)
    {
        response = "";
        return MsgType::INVALID;
    }
    response = std::to_string(list_size);
    return MsgType::INTEGER;
}

std::vector<std::string> RedisServer::handleListPop(const std::vector<std::string>& cmd)
{
    if(cmd.empty() || cmd.size() < 2)
        return {};

    int count =1;
    if( (cmd[0] == "LPOP") )
    {
        if(cmd.size() == 3)
        {
            count = std::stoi(cmd[2]);
            return global_store.list_pop_front_multi(cmd[1],count,false);
        }
        else 
        {
            return global_store.list_pop_front_multi(cmd[1],1,false);
        }
        
    }
    if(cmd[0] == "BLPOP")
    {
        if(cmd.size() == 3)
            return global_store.list_pop_front_multi(cmd[1],1,true);
    }
    return {};
}

std::vector<std::string> RedisServer::handleLeftRange(const std::vector<std::string>& cmd)
{
    if( (cmd.size() == 4) && (cmd[0]== "LRANGE") )
    {
        // TODO: can throw exception, replace with robust mechanism later
        int start = std::stoi(cmd[2]);
        int stop = std::stoi(cmd[3]);
        return global_store.getListRange(cmd[1],start,stop);
    }
    return {};
}

RedisServer::~RedisServer()
{
    if(server_fd >= 0)
        close(server_fd);
}

bool RedisServer::SharedStore::getValue(const std::string& key, std::string& value)
{
    if(kv_store.contains(key))
    {
        KeyRecord& record = kv_store[key];
        if( record.expiry.has_value() )
        {
            // NOTE: Lazy expiration strateigy
            auto now = std::chrono::steady_clock::now();
            if(now >= record.expiry)
            {
                // delete expired entry
                kv_store.erase(key);
                return false;
            }
        }
        value = record.value;
        return true;
    }
    return false;
}
        
void RedisServer::SharedStore::setValue(const std::string&key,const std::string& value, int expiry,ExpiryTimeUnit units)
{
    KeyRecord record;
    record.value = value;

    if(expiry != -1)
    {
        if(units == ExpiryTimeUnit::MILLISECONDS)
            record.expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(expiry);
        else
            record.expiry = std::chrono::steady_clock::now() + std::chrono::seconds(expiry);
    }
    kv_store[key] = std::move(record);
}


// Takes full command vector as input, and consumes all the list elements
int RedisServer::SharedStore::list_push_back(const std::string& list_name,const std::vector<std::string>& elements)
{
    if(list_name.empty())
        return -1;

    std::vector<std::string>& list = list_store[list_name];

    // permit empty element string for now, unless specified later
    int idx = 2;
    while(idx < elements.size())
    {
        list.push_back(std::move(elements[idx++]));
    }

    return list.size();
}

// Takes full command vector as input, and consumes all the list elements
int RedisServer::SharedStore::list_push_front(const std::string& list_name,const std::vector<std::string>& elements)
{
    if(list_name.empty())
        return -1;

    std::vector<std::string>& list = list_store[list_name];

    // permit empty element string for now, unless specified later
    int idx = 2;
    while(idx < elements.size())
    {
        list.insert(list.begin(),std::move(elements[idx++]));
    }

    return list.size();
}

std::vector<std::string> RedisServer::SharedStore::getListRange(const std::string& list_name,int start,int stop)
{
    if(list_name.empty())
        return {};
    
    auto it = list_store.find(list_name);
    if( it == list_store.end() )
        return {};
    
    const std::vector<std::string>& list = it->second;

    int list_size = (int)list.size();

    if( start<0 )
        start = std::max(0,start+list_size);
    if( stop<0 )
        stop = std::max(0,stop+list_size);

    stop = std::min(stop, list_size - 1) ;

    if( (start >= list_size) )
        return {};

    if( start > stop )
        return {};

    return std::vector(list.begin()+start, list.begin()+ stop+1);
}


std::size_t RedisServer::SharedStore::getListSize(const std::string& listname)
{
    if(listname.empty())
        return 0;

    auto it = list_store.find(listname);

    if(it == list_store.end()) 
        return 0;

    return it->second.size();
}

std::vector<std::string> RedisServer::SharedStore::list_pop_front_multi(const std::string& listname, int count, bool append_listname)
{
    if(listname.empty())
        return {};

    auto it = list_store.find(listname);

    if(it == list_store.end())
        return {};

    std::vector<std::string>& list = it->second;

    if(list.empty())
        return {};

    std::vector<std::string> elements;

    
    if(append_listname)
        elements.push_back(listname);

    while( count && !list.empty())
    {
        elements.push_back(std::move(list.front()));
        list.erase(list.begin());
        count--;
    }

    return elements;
}


void RedisServer::SharedStore::registerWaitingClient(const std::string listname, std::size_t client_idx, uint64_t connection_id)
{
    if(listname.empty())
        return;

    waiting_clients[listname].emplace(client_idx,connection_id);
}


bool RedisServer::SharedStore::list_pop_front(const std::string& listname, std::string& response )
{

    if(listname.empty())
        return false;

    auto it = list_store.find(listname);

    if(it == list_store.end())
        return false;

    std::vector<std::string>& list = it->second;

    if(list.empty())
        return false;
        
    response = list.front();
    list.erase(list.begin());

    return true;
}