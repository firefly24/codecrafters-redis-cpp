#pragma once

#include <string>
#include <unordered_map>

enum class Command
{
    PING,
    ECHO,
    GET,
    SET,
    RPUSH,
    LPUSH,
    LRANGE,
    LLEN,
    LPOP,
    BLPOP,
    UNKOWN
};

static const std::unordered_map<std::string, Command> commands = 
        {
            {"PING", Command::PING},
            {"ECHO", Command::ECHO},
            {"GET", Command::GET},
            {"SET", Command::SET},
            {"RPUSH", Command::RPUSH},
            {"LPUSH", Command::LPUSH},
            {"LRANGE", Command::LRANGE},
            {"LLEN", Command::LLEN},
            {"LPOP", Command::LPOP},
            {"BLPOP", Command::BLPOP}
        };

Command toCommand(const std::string& cmd_name)
{
    auto it = commands.find(cmd_name);

    return (it != commands.end())? it->second: Command::UNKOWN;
}