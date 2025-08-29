#pragma once

#include <string>

class JsonParameters;
namespace Json {
    class Value;
}

template<typename T>
void json_serialize(const std::string&, const T&, JsonParameters&, Json::Value&)
{
    throw "unimplemented serialization";
}

template<>
void json_serialize(const std::string& name, const int& value, JsonParameters&, Json::Value& out)
{
    out[name] = value;
}

template<>
void json_serialize(const std::string& name, const std::string& value, JsonParameters&, Json::Value& out)
{
    out[name] = value;
}

template<typename T, unsigned N>
void json_serialize(const std::string& name, const Array<T, N>& value, JsonParameters&, Json::Value& out)
{
    out[name] = value;
}
