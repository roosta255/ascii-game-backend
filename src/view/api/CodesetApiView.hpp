#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "CodeEnum.hpp"
#include "CodeType.hpp"
#include "Codeset.hpp"

struct CodesetApiView
{
    struct TableEntryView {
        std::string code;
        int value;
    };

    struct LogEntryView {
        std::string code;
        int value;
        bool isError;
    };

    std::vector<std::string> errors;
    std::vector<TableEntryView> table;
    std::vector<LogEntryView> logs;
    bool isAnyOverflow;
    bool isAnyFailure;

    CodesetApiView() = default;

    explicit CodesetApiView(const Codeset& codeset)
        : isAnyOverflow(codeset.isAnyOverflow)
        , isAnyFailure(codeset.isAnyFailure)
    {
        codeset.forEachTableEntry([&](const CodeEnum code, const int value) {
            if (code_to_type(code) == CodeType::ERROR) {
                errors.push_back(code_to_text(code));
            }
            table.push_back(TableEntryView{
                .code  = code_to_text(code),
                .value = value,
            });
        });

        codeset.forEachLog([&](const CodeEnum code, const int value) {
            logs.push_back(LogEntryView{
                .code    = code_to_text(code),
                .value   = value,
                .isError = code_to_type(code) == CodeType::ERROR,
            });
        });
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CodesetApiView::TableEntryView, code, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CodesetApiView::LogEntryView, code, value, isError)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CodesetApiView, errors, table, logs, isAnyOverflow, isAnyFailure)
