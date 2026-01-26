#include "Codeset.hpp"
#include <sstream>

Codeset::Codeset(){
    rackLogs.setEmpty();
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

bool Codeset::addSuccess(const bool input, const CodeEnum code) {
    if (input) {
        isAnySuccess = true;
        return true;
    }

    addLogAndTableIfError(error);
    if (error != code) {
        addLogAndTableIfError(code);
    }

    return false;
}

bool Codeset::addSuccessElseFailure(const bool input, const CodeEnum code) {
    if (input) {
        isAnySuccess = true;
        return true;
    }

    isAnyFailure = true;
    addLogAndTableIfError(error);
    if (error != code) {
        addLogAndTableIfError(code);
    }

    return false;
}

bool Codeset::addSuccessElseFailureIfCodedSuccess(const CodeEnum code) {
    return addSuccessElseFailure(code == CODE_SUCCESS);
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

bool Codeset::isAnySuccessfulWithoutFailures()const {
    return isAnySuccess && !isAnyFailure;
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
    output << "]\n\ttable {";
    i = 0;
    for (const auto& value: table) {
        if (value != 0) {
            output << " " << code_to_text(i) << "(" << value << ")";
        }
        i++;
    }
    output << "}\n\tlogs {";
    for (const auto& entry: rackLogs) {
        output << " " << code_to_text(entry.first) << (code_to_type(i) == CodeType::ERROR ? "(ERROR: " : "(") << entry.second << ")";
    }
    output << "}";

    return output.str();
}