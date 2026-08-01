#pragma once

#include <array>
#include <optional>
#include <sstream>

#include <catch2/catch_all.hpp>

#include "CodeEnum.hpp"
#include "Codeset.hpp"

// ── CodesetExpect ─────────────────────────────────────────────────────────────
// Sparse specification: unspecified fields are wildcards that always pass.
// Build one with chained setters or direct aggregate initialization.

struct CodesetExpect {
    // When set, asserts codeset.isLatestSuccessFlag equals the value.
    std::optional<bool> isLatestSuccessFlag;

    // When set, asserts codeset.error equals the value.
    std::optional<CodeEnum> error;

    // When set, asserts codeset.table[code] equals the value.
    std::array<std::optional<int>, CODE_COUNT> tableEquals = {};

    // When set, asserts codeset.table[code] does not equal the value.
    std::array<std::optional<int>, CODE_COUNT> tableNotEquals = {};

    CodesetExpect& expectIsLatestSuccessFlag(bool value) {
        isLatestSuccessFlag = value;
        return *this;
    }
    CodesetExpect& expectError(CodeEnum code) {
        error = code;
        return *this;
    }
    CodesetExpect& expectTable(CodeEnum code, int value) {
        tableEquals[code] = value;
        return *this;
    }
    CodesetExpect& expectTableNot(CodeEnum code, int value) {
        tableNotEquals[code] = value;
        return *this;
    }
    // Asserts every ERROR-type table index is 0 (mirrors getErrorTable() == getEmptyTable()).
    CodesetExpect& expectNoErrors() {
        for (int i = 0; i < CODE_COUNT; i++) {
            if (code_to_type(i) == CodeType::ERROR) {
                tableEquals[i] = 0;
            }
        }
        return *this;
    }
};

// Let Catch2 print the full describe() dump whenever a Codeset appears in a REQUIRE_THAT expansion.
namespace Catch {
template<>
struct StringMaker<Codeset> {
    static std::string convert(const Codeset& c) { return c.describe(); }
};
} // namespace Catch

// ── Catch2 Matcher ────────────────────────────────────────────────────────────

class CodesetMatcher : public Catch::Matchers::MatcherBase<Codeset> {
    CodesetExpect _expect;
public:
    explicit CodesetMatcher(CodesetExpect expect) : _expect(std::move(expect)) {}

    bool match(const Codeset& actual) const override {
        if (_expect.isLatestSuccessFlag && actual.isLatestSuccessFlag != *_expect.isLatestSuccessFlag)
            return false;
        if (_expect.error && actual.error != *_expect.error)
            return false;
        for (int i = 0; i < CODE_COUNT; i++) {
            if (!_expect.tableEquals[i]) continue;
            if (actual.getTableValue(CodeEnum(i)) != *_expect.tableEquals[i])
                return false;
        }
        for (int i = 0; i < CODE_COUNT; i++) {
            if (!_expect.tableNotEquals[i]) continue;
            if (actual.getTableValue(CodeEnum(i)) == *_expect.tableNotEquals[i])
                return false;
        }
        return true;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "satisfies CodesetExpect{\n";
        if (_expect.isLatestSuccessFlag)
            ss << "  isLatestSuccessFlag = " << (*_expect.isLatestSuccessFlag ? "true" : "false") << "\n";
        if (_expect.error)
            ss << "  error = " << code_to_text(*_expect.error) << "\n";
        for (int i = 0; i < CODE_COUNT; i++) {
            if (!_expect.tableEquals[i]) continue;
            ss << "  table[" << code_to_text(i) << "] == " << code_to_value_text(i, *_expect.tableEquals[i]) << "\n";
        }
        for (int i = 0; i < CODE_COUNT; i++) {
            if (!_expect.tableNotEquals[i]) continue;
            ss << "  table[" << code_to_text(i) << "] != " << code_to_value_text(i, *_expect.tableNotEquals[i]) << "\n";
        }
        ss << "}";
        return ss.str();
    }
};

inline CodesetMatcher MatchesCodesetExpect(CodesetExpect expect) {
    return CodesetMatcher(std::move(expect));
}
