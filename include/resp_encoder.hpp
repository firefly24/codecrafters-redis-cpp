#pragma once

#include <string>

class RESPEncoder
{

 private:
    std::string prefix;
    std::string suffix;
    std::string separator;
 public:
    
    explicit RESPEncoder();
    std::string encodeMsg(std::string msg);

    // Delete copy 
    //RESPEncoder& operator=(const RESPEncoder&) = delete;
    //RESPEncoder(const RESPEncoder&) = delete;

    //Delete move
   // RESPEncoder& operator=(const RESPEncoder&&) = delete;
    //RESPEncoder(const RESPEncoder&&) = delete;
};





