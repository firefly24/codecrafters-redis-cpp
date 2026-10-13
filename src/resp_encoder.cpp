#include <string>

#include "resp_encoder.hpp"

std::string RESPEncoder::separator = "\r\n";

std::string RESPEncoder::encodeMsg(std::string msg)
{
    if(msg == "PING")
        return "+" + msg + separator;

    return "$" + std::to_string(msg.length()) + separator + msg + separator ; 
}