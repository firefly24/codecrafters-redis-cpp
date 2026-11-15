#pragma once 
#include <string>
#include <vector>

enum class MsgType
{
    SIMPLE,
    BULK,
    EMPTY,
    NULL_ARRAY,
    INTEGER,
    ARRAY,
    DEFER,
    INVALID 
};

struct CommandResult
{
    MsgType type;
    std::string result;
    std::vector<std::string> result_array;

    CommandResult(): type(MsgType::INVALID),result("")
    {}
};