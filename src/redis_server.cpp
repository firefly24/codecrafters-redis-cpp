#include <chrono>
#include <climits>
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
#include <algorithm>

#include "redis_server.hpp"
#include "client_connection.hpp"
#include "command_types.hpp"
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
    
    //clients.push_back(std::make_unique<ClientConnection>(client_fd, *this));
    clients.emplace_back(std::make_shared<ClientConnection>(client_fd, *this));

    std::cout << "Client connected\n";
    return true;
}

int RedisServer::generateValidFds(std::vector<pollfd>& poll_fds)
{
    poll_fds.clear();   
    poll_fds.push_back({server_fd,POLLIN, 0});

    int shortest_timeout = INT_MAX;

    // remove invalid/ closed client connections 
    //std::erase_if(clients,[](const std::unique_ptr<ClientConnection>& conn){return !conn->isValid();});
    std::erase_if(clients,[](const std::shared_ptr<ClientConnection>& conn){return !conn->isValid();});
    for (const auto& client_connection: clients)
    {
        short events;
        if(client_connection->isBlocked())
        {
            events = 0;
        }
        else
            events = POLLIN;

        int timeout = client_connection->getTimeoutMs();

        if( timeout != -1)
        {
            shortest_timeout = std::min(timeout,shortest_timeout);
        }
        
        poll_fds.push_back({client_connection->getFd(),events,0});
    }

    return (shortest_timeout==INT_MAX)?-1:shortest_timeout;
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
        int timeout = generateValidFds(poll_fds);

        int ready_count = poll(poll_fds.data(), poll_fds.size(), timeout);
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
            //std::cerr << "POLLIN: blocked=" << clients[i-1]->isBlocked() << std::endl;
            if(clients[i-1]->isBlocked())
            {
                if(clients[i-1]->getTimeoutMs() == 0)
                {
                    clients[i-1]->createAndSendResponse({}, MsgType::NULL_ARRAY);
                    clients[i-1]->unblock();
                }
            }
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

CommandResult RedisServer::processCommand( const std::vector<std::string>& cmd)
{
    CommandResult response;

    if(cmd.empty())
    {
        response.type = MsgType::EMPTY;
        return response;
    }

    Command command = toCommand(cmd[0]);
    
    switch (command) 
    {
        case Command::PING:
        {
            response.result = "PONG";
            response.type = MsgType::SIMPLE;
            break;
        }
        case Command::ECHO:
        {
            if(cmd.size() == 2)
            {
                response.result = cmd[1];
                response.type = MsgType::BULK;
            }
            break;
        }
        case Command::GET:
        {
            if(cmd.size() == 2)
            {
                if(global_store.getValue(cmd[1],response.result))
                    response.type = MsgType::BULK;
                else
                    response.type =  MsgType::EMPTY;
            }
            break;
        }
        case Command::SET:
        {
            response.type = handleSetKey(cmd,response.result);
            break;
        }
        case Command::RPUSH:
        {
            return handleListPush(false,cmd);
        }
        case Command::LPUSH:
        {
            return handleListPush(true,cmd);
        }
        case Command::LRANGE:
        {
            if(cmd.size() == 4 )
            {
                // TODO: can throw exception, replace with robust mechanism later
                int start = std::stoi(cmd[2]);
                int stop = std::stoi(cmd[3]);

                return handleLeftRange(cmd[1],start,stop);
            }
            break;
        }
        case Command::LLEN:
        {
            if (cmd.size() == 2)
                return handleListLength(cmd[1]);
            else
                return handleListLength("");
        }
        case Command::LPOP:
        { 
            response.type = MsgType::EMPTY;

            if(cmd.size() >= 2)
            {
                int expected_multi_count = 0;
                if(cmd.size() == 3)
                    expected_multi_count = std::stoi(cmd[2]);

                return handleListPop(cmd[1],false,expected_multi_count);


                /*
                if(cmd.size() == 2)
                {
                    if(response.result_array.size())
                    {
                        response.result = response.result_array[0];
                        response.type = MsgType::BULK;
                    }
                }
                else
                {
                    response.type = MsgType::ARRAY;
                }
                */
            }
            break;
        }
        case Command::BLPOP:
        {
            // Since the server execution is single threaded for now,
            // this operation just returns if an element is available for immediate pop without actually popping
            // The actual pop happens in the second call - ProcessCommandComplex, 
            // and no other client modifies the list inbetween for single threaded process
            
            if(cmd.size() == 3)
            {
                CommandResult response =  handleListPop(cmd[1],true,1);
                if(response.type == MsgType::DEFER)
                    response.result = cmd[2];
                return response;
            }
            break;
        }
        default:
        {
            return response;
        }
    }

    // std::cout << cmd[0] << std::endl;
    return response;
}

MsgType RedisServer::handleSetKey(const std::vector<std::string>& cmd, std::string& response)
{
    if(cmd.size() >=3)
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

CommandResult RedisServer::handleListLength(const std::string& listname)
{
    CommandResult response;

    std::size_t list_size = global_store.getListSize(listname);

    response.result = std::to_string(list_size);
    response.type = MsgType::INTEGER;
    return response;
}

CommandResult RedisServer::handleListPush(bool front,const std::vector<std::string>& cmd)
{
    CommandResult response;
    int list_size = -1;
    if ( (cmd.size() >= 3) )
    {
        if(front)
            list_size = global_store.list_push_front(cmd[1], cmd);
        else
            list_size = global_store.list_push_back(cmd[1], cmd);
    }
    if(list_size != -1)
    {
        response.result = std::to_string(list_size);
        response.type = MsgType::INTEGER;
    }
    return response;
}

CommandResult RedisServer::handleListPop(const std::string& listname,bool is_blocking,int array_count)
{
    CommandResult response;
    response.type = MsgType::EMPTY;

    array_count = (is_blocking)?1:array_count;

    response.result_array =  global_store.list_pop_front(listname,is_blocking,array_count);

    if(!response.result_array.size())
        return response;
    
    // single non-blocking pop
    if(array_count == 0)
    {
        response.result = response.result_array[0];
        response.type = MsgType::BULK;
    }
    else 
    {
        response.type = MsgType::ARRAY;
        if(is_blocking && (response.result_array.size() == 1) )
            response.type = MsgType::DEFER;
    }
    return response;
}

/*
std::vector<std::string> RedisServer::handleListPop(const std::vector<std::string>& cmd,bool is_blocking)
{
    if(cmd.empty() || cmd.size() < 2)
        return {};

    int count =1;
    if( is_blocking )
    {
        if(cmd.size() == 3)
        {
            //std::cout << cmd[1] << std::endl;
            return global_store.list_pop_front(cmd[1],1,true);
        }
    }
    else
    {
        if(cmd.size() == 3)
            count = std::stoi(cmd[2]);
        
        return global_store.list_pop_front(cmd[1],count,false);
        
    }
    return {};
}
*/
CommandResult RedisServer::handleLeftRange(const std::string& listname,int start, int stop)
{
    CommandResult response;
    response.result_array = global_store.getListRange(listname,start,stop);
    response.type = MsgType::ARRAY;

    return response;
}

void RedisServer::requestWaiterRegistration(const std::string& listname,BlockedClientRef client)
{
    global_store.registerWaitingClient(listname, client);
}

std::shared_ptr<ClientConnection> RedisServer::getNextWaitingClient(const std::string& listname)
{
    return global_store.getNextBlockedRef(listname);
}

void RedisServer::wakeBlockedClient(std::string& listname)
{
    std::shared_ptr<ClientConnection> client = getNextWaitingClient(listname);
    
    if(!client)
        return;

    auto result_array = global_store.list_pop_front(listname,1,true);

    client->createAndSendResponse(result_array,MsgType::ARRAY);

    //std::string reply = client->encodeResponse(response);
    //client->sendResponse(reply);
    
    client->unblock();
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

std::vector<std::string> RedisServer::SharedStore::list_pop_front(const std::string& listname, bool isblocking,int count)
{
    if(listname.empty())
        return {};

    std::vector<std::string> elements = {};

    if(isblocking)
        elements.push_back(listname);

    auto it = list_store.find(listname);

    if(it != list_store.end())
    {
        std::vector<std::string>& list = it->second;
        count = std::max(count,1);
        while( count && !list.empty())
        {
            elements.push_back(std::move(list.front()));
            list.erase(list.begin());
            count--;
        }
    }
    return elements;
}


void RedisServer::SharedStore::registerWaitingClient(const std::string& listname, BlockedClientRef client)
{
    if(listname.empty())
        return;

    waiting_clients[listname].push(std::move(client));
}

std::shared_ptr<ClientConnection> RedisServer::SharedStore::getNextBlockedRef(const std::string& listname)
{
    auto it = waiting_clients.find(listname);
    if(it != waiting_clients.end())
    {
        std::queue<BlockedClientRef>& waiters = it->second;
        while(!waiters.empty())
        {
            auto client = waiters.front().lock();
            waiters.pop();

            if(client && client->isBlocked())
                return client;
        }
    }
    return nullptr;
}