#include <string>

#include "resp_encoder.hpp"


RESPEncoder::RESPEncoder()
{
    prefix = "+";
    suffix = "\r\n";

    separator = "\r\n";
}

std::string RESPEncoder::encodeMsg(std::string msg)
{
    return "$" + std::to_string(msg.length()) + separator + msg + separator ; 
   // return prefix + msg + suffix;
}