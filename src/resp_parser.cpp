#include "resp_parser.hpp"
#include <charconv>
#include <string>
#include <string_view>
#include <iostream>

std::string RESPParser::delimiter = "\r\n";

std::vector<std::string> RESPParser::parse(std::string input)
{
    std::size_t pos,begin=0;

    if(input.empty())
        return {};

    pos = input.find("\r\n",begin);
    if(pos!= std::string::npos)
    {
        std::string array_header = input.substr(begin,pos-begin);
        
        if(!array_header.empty())
        {
            int num_words=1;
            if(array_header.front() == '*')
            {
                num_words = std::stoi(array_header.substr(1));
                input = input.substr(pos+2);
            }
            return parseLineHelper(input,num_words);
        }
    }

    return {};
}

std::vector<std::string> RESPParser::parseLineHelper(std::string input, int num_words)
{
    std::vector<std::string> in_words;

    std::size_t pos,begin = 0;

    while(num_words)
    {   
        pos = input.find("\r\n", begin);
        if(pos == std::string::npos)
            return {};

        std::string bulk_header = input.substr(begin,pos-begin);

        // parse length
        if(bulk_header.empty() || (bulk_header.front() != '$') )
            return {};

        int wordlen  = std::stoi(bulk_header.substr(1));

        // extract word
        begin = pos+2;
        if(begin > input.length())
            return {};

        pos = input.find("\r\n", begin);
        if(pos == std::string::npos)
            return {};
        
        std::string payload = input.substr(begin,pos-begin);
        
        if(payload.length() != wordlen)
        {
            std::cerr << "payload length mismatch observed : "<< payload 
                     <<"(" << payload.length() << ") expected: " << wordlen << std::endl;
            return {};
        }
        
        in_words.push_back(payload);
        begin += wordlen+2;

        num_words--;
    }
    return in_words;
}