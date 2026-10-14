#include <string>
#include "resp_encoder.hpp"
#include "message_types.hpp"

std::string RESPEncoder::separator = "\r\n";
std::string RESPEncoder::prefix = "+";

std::string RESPEncoder::encodeMsg(std::string msg, MsgType encode_type)
{
    if(encode_type == MsgType::SIMPLE)
    {
        return prefix + msg + separator;
    }

    if(encode_type == MsgType::BULK)
    {
        return "$" + std::to_string(msg.length()) + separator + msg + separator;
    }
    if(encode_type == MsgType::EMPTY)
    {
        return "$-1" + separator;
    }
    if(encode_type == MsgType::INTEGER)
    {
        return ":" + msg + separator;
    }
    if(encode_type == MsgType::INVALID)
        return "";
    return "";
}

std::string RESPEncoder::encodeMsgArray(std::vector<std::string>& array)
{
    std::string response = "*" + std::to_string(array.size()) + separator;
    
    for(const auto& word: array)
    {
        response += "$" + std::to_string(word.size()) + separator + word + separator;
    }
    return response;
}


