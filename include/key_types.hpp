#pragma once
#include <string>
#include <unordered_map>

enum class KeyType{
    STRING,
    LIST,
    SET,
    ZSET,
    HASH,
    STREAM,
    VECTORSET,
    NONE
};

inline std::string toTypeName(const KeyType& type)
{
    static const std::unordered_map<KeyType,std::string> key_type_names = 
        {
            {KeyType::STRING,"string"},
            {KeyType::LIST,"list"},
            {KeyType::SET,"set"},
            {KeyType::ZSET,"zset"},
            {KeyType::HASH,"hash"},
            {KeyType::STREAM,"stream"},
            {KeyType::VECTORSET,"vectorset"},
            {KeyType::NONE, "none"}
        };
    auto it = key_type_names.find(type);

    return (it != key_type_names.end())? it->second: "none";
}