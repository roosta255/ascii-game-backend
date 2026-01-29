#pragma once

#include "Array.hpp"
#include "CodeEnum.hpp"
#include "Rack.hpp"
#include <utility>

class Codeset
{
private:
    static constexpr auto MAX_STORED_LOGS = 64;
    Array<std::pair<CodeEnum, int>, MAX_STORED_LOGS> arrayLogs;
    Rack<std::pair<CodeEnum, int>> rackLogs = arrayLogs;
    Array<int, CODE_COUNT> table;

public:
    static const Array<int, CODE_COUNT>& getEmptyTable();
    Codeset();
    CodeEnum error = CODE_UNSET;
    bool isAnyOverflow = false, isAnyFailure = false;

    // returns the input
    bool addFailure(const bool input, const CodeEnum code = CODE_UNSET);

    void addLog(const CodeEnum& input, int value = 1);
    void addError(const CodeEnum& input, int value = 1);
    void addTable(const CodeEnum& input, int value = 1);
    bool addUnreachableError(const int line);
    void setTable(const CodeEnum& input, int value = 1);
    void addLogAndTableIfError(const CodeEnum& input);

    CodeEnum findErrorInTable()const;
    std::string describe()const;
    std::string describe(const std::string& prefix)const;
    Array<int, CODE_COUNT> getErrorTable()const;
};

std::ostream& operator<<(std::ostream& os, const Array<int, CODE_COUNT>& table);
bool operator==(const Array<int, CODE_COUNT>& table, const Array<int, CODE_COUNT>& table2);
bool operator!=(const Array<int, CODE_COUNT>& table, const Array<int, CODE_COUNT>& table2);
