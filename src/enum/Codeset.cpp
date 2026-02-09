#include "Codeset.hpp"
#include <sstream>

Codeset::Codeset(){
    rackLogs.setEmpty();
}

bool Codeset::addDebugLine(const int line) {
    addLog(CODE_DEBUG_LINE_REACHED);
    setTable(CODE_DEBUG_LINE, line);
    return false;
}

void Codeset::addLog(const CodeEnum& input, int value ) {
    if (rackLogs.isFull()) {
        isAnyOverflow = true;
        return;
    }

    rackLogs.push_back(std::make_pair(input, value));
}

void Codeset::addLogAndTableIfError(const CodeEnum& input) {
    if (code_to_type(input) == CodeType::ERROR) {
        addLog(input, 1);
        addTable(input, 1);
    }
}

bool Codeset::addFailure(const bool input, const CodeEnum code) {
    if (input) {
        isAnyFailure = true;
        addLogAndTableIfError(error);
        if (error != code) {
            addLogAndTableIfError(code);
        }
        return true;
    }

    return false;
}

void Codeset::addError(const CodeEnum& input, int value) {
    isAnyFailure = true;
    addLogAndTableIfError(error);
    if (error != input) {
        addLogAndTableIfError(input);
    }
}

void Codeset::addTable(const CodeEnum& input, int value) {
    table.access(input, [&](int& current){
        current += value;
    });
}

bool Codeset::addUnreachableError(const int line) {
    addError(CODE_UNREACHABLE_CODE_WAS_REACHED);
    setTable(CODE_UNREACHABLE_CODE_LINE, line);
    return false;
}

void Codeset::setTable(const CodeEnum& input, int value) {
    table.access(input, [&](int& current){
        current = value;
    });
}

CodeEnum Codeset::findErrorInTable()const {
    auto i = 0;
    for (const auto& value: table) {
        if (value > 0 && code_to_type(i) == CodeType::ERROR) {
            return (CodeEnum)i;
        }
        i++;
    }
    return CODE_UNSET;
}

std::string Codeset::describe(const std::string& prefix) const {
    std::stringstream output;
    output << prefix << "[";
    auto i = 0;
    for (const auto& value: table) {
        if (value > 0 && code_to_type(i) == CodeType::ERROR) {
            output << " " << code_to_text(i);
        }
        i++;
    }
    output << " ]\n\ttable {";
    i = 0;
    for (const auto& value: table) {
        if (value != 0) {
            output << " " << code_to_text(i) << "(" << value << ")";
        }
        i++;
    }
    output << " }\n\tlogs {";
    for (const auto& entry: rackLogs) {
        output << " " << code_to_text(entry.first) << (code_to_type(i) == CodeType::ERROR ? "(ERROR: " : "(") << entry.second << ")";
    }
    output << " }";

    return output.str();
}

std::string Codeset::describe() const {
    return describe("");
}

Array<int, CODE_COUNT> Codeset::getErrorTable()const {
    Array<int, CODE_COUNT> result;
    auto i = 0;
    for (const auto& value: table) {
        if (value != 0 && code_to_type(i) == CodeType::ERROR) {
            result.assignValue(i, value);
        }
        i++;
    }
    return result;
}

const Array<int, CODE_COUNT>& Codeset::getEmptyTable() {
    static Array<int, CODE_COUNT> table;
    return table;
}

std::ostream& operator<<(std::ostream& os, const Array<int, CODE_COUNT>& table) {
    auto i = 0;
    os << "[";
    for (const auto& value: table) {
        if (value != 0) {
            os << " " << code_to_text(i) << "(" << value << ")";
        }
        i++;
    }
    os << " ]";
    return os;
}

bool operator==(const Array<int, CODE_COUNT>& left, const Array<int, CODE_COUNT>& right) {
    auto i = 0;
    for (const auto& value: left) {
        if (value != right.getOrDefault(i, 0)) {
            return false;
        }
        i++;
    }
    return true;
}

bool operator!=(const Array<int, CODE_COUNT>& left, const Array<int, CODE_COUNT>& right) {
    return !(left == right);
}
