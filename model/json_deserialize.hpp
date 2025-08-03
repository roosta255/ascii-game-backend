#pragma once

class JsonParameters;
namespace Json {
    class Value;
}

template<typename T>
bool json_deserialize(T& t, Json::Value& out)
{
    throw "unimplemented deserialization";
}
