#pragma once

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "Rack.hpp"
#include <utility>

class Codeset
{
private:
    static constexpr auto MAX_STORED_LOGS = 32;
    Array<std::pair<CodeEnum, int>, MAX_STORED_LOGS> arrayLogs;
    Rack<std::pair<CodeEnum, int>> rackLogs = arrayLogs;
    Array<int, CODE_COUNT> table;

public:
    Codeset();
    CodeEnum error = CODE_UNSET;
    bool isAnySuccess = false, isAnyOverflow = false, isAnyFailure = false;

    // returns the input
    bool addSuccess(const bool input, const CodeEnum code = CODE_UNSET);
    bool addSuccessElseFailure(const bool input, const CodeEnum code = CODE_UNSET);
    bool addSuccessElseFailureIfCodedSuccess(const CodeEnum code);
    bool addFailure(const bool input, const CodeEnum code = CODE_UNSET);

    void addLog(const CodeEnum& input, int value = 1);
    void addError(const CodeEnum& input, int value = 1);
    void addTable(const CodeEnum& input, int value = 1);
    void setTable(const CodeEnum& input, int value = 1);
    void addLogAndTableIfError(const CodeEnum& input);

    CodeEnum findErrorInTable()const;
    bool isAnySuccessfulWithoutFailures()const;
    std::string describe(const std::string& prefix)const;

};