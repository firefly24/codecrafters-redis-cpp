#include <stdexcept>
#include <string>
#include "resp_encoder.hpp"
#include "message_types.hpp"
#include <stdexcept>

std::string RESPEncoder::separator = "\r\n";
std::string RESPEncoder::prefix = "+";

std::string RESPEncoder::encodeMsg(std::string msg, MsgType encode_type)
{
    if(encode_type == MsgType::SIMPLE)
    {
        return "+" + msg + separator;
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
    if(encode_type == MsgType::NULL_ARRAY)
    {
        return "*-1" + separator;
    }
    if(encode_type == MsgType::DEFER)
        throw std::runtime_error("DEFER should never be encoded");
    return "";
}

std::string RESPEncoder::encodeMsgArray(const std::vector<std::string>& array)
{
    std::string response = "*" + std::to_string(array.size()) + separator;
    
    for(const auto& word: array)
    {
        response += "$" + std::to_string(word.size()) + separator + word + separator;
    }
    return response;
}


