#include "resp_parser.hpp"
#include <charconv>
#include <string>
#include <string_view>
#include <iostream>

std::string RESPParser::delimiter = "\r\n";

std::vector<std::string> RESPParser::parse(std::string input)
{
   // std::vector<std::string> in_words;
    std::size_t pos,begin=0;

    if(input.empty())
        return {};

    pos = input.find("\r\n",begin);

    //std::cout << pos << "  " << input<<  std::endl;

    if(pos!= std::string::npos)
    {
        std::string word = input.substr(begin,pos);
        //std::cout << word << "  " << word<<  std::endl;
        
        if(!word.empty())
        {
            int num_words=1;
            if(word[0] == '*')
            {
                //auto result = std::from_chars(word.data(), word.data() + word.size(), num_words);
                num_words = std::stoi(word.substr(1));
                std::cout << "Num words: " << num_words <<std::endl;
                input = input.substr(pos+2);
                // TODO: how to check failures and return;
            }
            return parseLineHelper(input,num_words);
        }
    }

    return {};
}

std::vector<std::string> RESPParser::parseLineHelper(std::string input, int num_words)
{
    std::vector<std::string> in_words;

    //std::cout << input << std::endl;

    std::size_t pos,begin = 0;

    while(num_words)
    {   
        pos = input.find("\r\n", begin);
        if(pos == std::string::npos)
            return {};

        std::string word = input.substr(begin,pos-begin);

        // parse length
        if(word.empty() || (word[0] != '$') )
            return {};

        int wordlen  = std::stoi(word.substr(1));

        // extract word
        begin = pos+2;
        //std::cout << word << "  **" <<begin << std::endl;
        if(begin > input.length())
            return {};

        pos = input.find("\r\n", begin);
        if(pos == std::string::npos)
            return {};
        
        word = input.substr(begin,wordlen);
        
        in_words.push_back(word);
        begin += wordlen+2;
        //std:: cout << wordlen << ": " << word << "  **" << begin << std::endl;
        num_words--;
    }
    return in_words;
}