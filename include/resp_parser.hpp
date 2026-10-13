#pragma once

#include <string>
#include <string_view>
#include <vector>

class RESPParser
{

private:
    static std::string delimiter;
public:
    RESPParser() = delete;
    static std::vector<std::string> parse(std::string input);
    static std::vector<std::string> parseLineHelper(std::string input, int num_words);
    static std::string extractRESPToken(std::string &input, std::size_t& begin);
};