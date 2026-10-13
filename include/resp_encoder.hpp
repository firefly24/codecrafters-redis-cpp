#pragma once

#include <string>

class RESPEncoder
{

 private:
//   static std::string prefix;
//   static std::string suffix;
   static std::string separator;
 public:
    
   RESPEncoder() = delete;
   static std::string encodeMsg(std::string msg);

    // Delete copy 
    //RESPEncoder& operator=(const RESPEncoder&) = delete;
    //RESPEncoder(const RESPEncoder&) = delete;

    //Delete move
   // RESPEncoder& operator=(const RESPEncoder&&) = delete;
    //RESPEncoder(const RESPEncoder&&) = delete;
};





