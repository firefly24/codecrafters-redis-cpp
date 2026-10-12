#pragma once

#include <string>
//#include <iostream>

class RESPEncoder
{

 private:
    std::string prefix;
    std::string suffix;
 public:
    
    explicit RESPEncoder();
    std::string encodeMsg(std::string msg);

    // Delete copy 
    RESPEncoder& operator=(const RESPEncoder&) = delete;
    RESPEncoder(const RESPEncoder&) = delete;

    //Delete move
    RESPEncoder& operator=(const RESPEncoder&&) = delete;
    RESPEncoder(const RESPEncoder&&) = delete;
};

RESPEncoder::RESPEncoder()
{
    prefix = "+";
    suffix = "\r\n";
}

std::string RESPEncoder::encodeMsg(std::string msg)
{
    return prefix + msg + suffix;
}





