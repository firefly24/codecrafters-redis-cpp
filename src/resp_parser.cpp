#include "resp_parser.hpp"
#include <string>
#include <iostream>

std::string RESPParser::delimiter = "\r\n";

std::vector<std::string> RESPParser::parse(std::string input)
{
    std::size_t pos;
    std::size_t begin=0;

    if(input.empty())
        return {};

    pos = input.find(delimiter);
    if(pos!= std::string::npos)
    {
        std::string array_header = input.substr(0,pos);
        
        if(!array_header.empty())
        {
            int num_words=1;
            if(array_header.front() == '*')
            {
                num_words = std::stoi(array_header.substr(1));
                begin = pos + delimiter.length();
            }
            return parseLineHelper(input,begin,num_words);
        }
    }

    return {};
}

std::vector<std::string> RESPParser::parseLineHelper(std::string& input, std::size_t begin, int num_words)
{
    std::vector<std::string> in_words;
    in_words.reserve(num_words);

    if(begin >= input.length())
        return {};

    while(num_words > 0 )
    {   
        std::string bulk_header;
        std::string payload;

        bulk_header = extractRESPToken(input, begin);
        if (begin == std::string::npos)
            return {};

        // parse length
        if(bulk_header.empty() || (bulk_header.front() != '$') )
            return {};

        int payload_length  = std::stoi(bulk_header.substr(1));

        // extract payload
        payload = extractRESPToken(input,begin);

        if(begin == std::string::npos)
            return {};

        if(payload.length() == payload_length)
        {
            in_words.push_back(payload);
        }
        else 
        {
            std::cerr << "payload length mismatch observed : "<< payload 
                <<"(" << payload.length() << ") expected: " << payload_length << std::endl;
            return {};
        }
        num_words--;
    }
    return in_words;
}

std::string RESPParser::extractRESPToken(std::string &input, std::size_t& begin)
{
    if(!input.empty() && (begin < input.length()) )
    {
        std::size_t pos = input.find(delimiter, begin);
        if(pos != std::string::npos)
        {
            std::string token = input.substr(begin, pos-begin);
            begin = pos + delimiter.length();
            return token;
        }
    }
    begin = std::string::npos;
    return {};
}