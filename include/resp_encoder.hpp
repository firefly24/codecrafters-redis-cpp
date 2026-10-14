#pragma once

#include <string>
#include "message_types.hpp"

//class MsgType;

class RESPEncoder
{

 private:
   static std::string prefix;
   static std::string separator;
 public:
    
   RESPEncoder() = delete;
   static std::string encodeMsg(std::string msg, MsgType encode_type);

    // Delete copy 
    //RESPEncoder& operator=(const RESPEncoder&) = delete;
    //RESPEncoder(const RESPEncoder&) = delete;

    //Delete move
   // RESPEncoder& operator=(const RESPEncoder&&) = delete;
    //RESPEncoder(const RESPEncoder&&) = delete;
};





