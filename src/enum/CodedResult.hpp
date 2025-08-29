#pragma once

#include "CodeEnum.hpp"
#include "Maybe.hpp"

template<typename T>
struct CodedResult {
    const char* line = __LINE__;
    const char* file = __FILE__;
    bool isSuccess = false;
    CodeEnum code = 0;
    Maybe<T> result;
};
