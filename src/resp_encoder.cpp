#include <string>

#include "resp_encoder.hpp"


RESPEncoder::RESPEncoder()
{
    prefix = "+";
    suffix = "\r\n";
}

std::string RESPEncoder::encodeMsg(std::string msg)
{
    return prefix + msg + suffix;
}