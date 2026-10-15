#include "match_record.hpp"
#include "../domain/serialization.hpp"

namespace hider_seeker {

namespace {

constexpr char HEX_DIGITS[] = "0123456789abcdef";

std::string toHex(const std::vector<uint8_t>& bytes) {
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (const uint8_t byte : bytes) {
        hex.push_back(HEX_DIGITS[byte >> 4]);
        hex.push_back(HEX_DIGITS[byte & 0x0F]);
    }
    return hex;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; // uppercase is rejected: the format is lowercase hex
}

bool fromHex(const std::string& hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) {
        return false;
    }
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t pos = 0; pos < hex.size(); pos += 2) {
        const int high = hexValue(hex[pos]);
        const int low = hexValue(hex[pos + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        bytes.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    out = std::move(bytes);
    return true;
}

} // namespace

nlohmann::json MatchRecord::toJson() const {
    std::vector<uint8_t> bytes;
    saveMatchState(state, bytes);

    nlohmann::json accountsJson = nlohmann::json::array();
    for (const auto& account : accounts) {
        accountsJson.push_back(account.empty() ? nlohmann::json(nullptr) : nlohmann::json(account));
    }

    nlohmann::json idempotencyJson = nlohmann::json::array();
    for (const auto& entry : idempotency) {
        idempotencyJson.push_back({
            {"account", entry.account},
            {"key", entry.key},
            {"request", entry.request},
            {"status_code", entry.status_code},
            {"response", entry.response}
        });
    }

    nlohmann::json movesJson = nlohmann::json::array();
    for (const auto& entry : moves) {
        movesJson.push_back({
            {"move_id", entry.move_id},
            {"account", entry.account},
            {"hider_id", entry.hider_id}
        });
    }

    nlohmann::json requestLogJson = nlohmann::json::array();
    for (const auto& entry : request_log) {
        requestLogJson.push_back({
            {"revision_before", entry.revision_before},
            {"account", entry.account},
            {"kind", entry.kind},
            {"body", entry.body}
        });
    }

    nlohmann::json publicEventsJson = nlohmann::json::array();
    for (const auto& entry : public_events) {
        publicEventsJson.push_back(hider_seeker::toJson(entry));
    }

    return {
        {"filename", filename},
        {"version", version},
        {"host", host},
        {"state", toHex(bytes)},
        {"accounts", accountsJson},
        {"next_move_id", next_move_id},
        {"idempotency", idempotencyJson},
        {"moves", movesJson},
        {"request_log", requestLogJson},
        {"public_events", publicEventsJson}
    };
}

bool MatchRecord::fromJson(const nlohmann::json& j) {
    // Parse into a scratch record and only commit once everything is valid.
    MatchRecord parsed;
    try {
        if (!j.is_object()) {
            return false;
        }

        std::vector<uint8_t> bytes;
        if (!fromHex(j.at("state").get<std::string>(), bytes)) {
            return false;
        }
        // loadMatchState rejects every non-current schema version itself.
        if (!loadMatchState(bytes, parsed.state)) {
            return false;
        }

        parsed.filename = j.at("filename").get<std::string>();
        parsed.version = j.at("version").get<int>();
        parsed.host = j.at("host").get<std::string>();
        parsed.next_move_id = j.at("next_move_id").get<uint32_t>();

        const auto& accountsJson = j.at("accounts");
        if (!accountsJson.is_array() || accountsJson.size() != MAX_HIDERS) {
            return false;
        }
        int slot = 0;
        for (auto& account : parsed.accounts) {
            const auto& value = accountsJson.at(slot++);
            if (value.is_null()) {
                account.clear();
            } else {
                account = value.get<std::string>();
            }
        }

        for (const auto& entryJson : j.at("idempotency")) {
            IdempotencyEntry entry;
            entry.account = entryJson.at("account").get<std::string>();
            entry.key = entryJson.at("key").get<std::string>();
            entry.request = entryJson.at("request").get<std::string>();
            entry.status_code = entryJson.at("status_code").get<int>();
            entry.response = entryJson.at("response");
            parsed.idempotency.push_back(std::move(entry));
        }

        for (const auto& entryJson : j.at("moves")) {
            MoveOwnershipEntry entry;
            entry.move_id = entryJson.at("move_id").get<std::string>();
            entry.account = entryJson.at("account").get<std::string>();
            entry.hider_id = entryJson.at("hider_id").get<uint8_t>();
            parsed.moves.push_back(std::move(entry));
        }

        for (const auto& entryJson : j.at("request_log")) {
            RequestLogEntry entry;
            entry.revision_before = entryJson.at("revision_before").get<uint64_t>();
            entry.account = entryJson.at("account").get<std::string>();
            entry.kind = entryJson.at("kind").get<std::string>();
            entry.body = entryJson.at("body");
            parsed.request_log.push_back(std::move(entry));
        }

        // Milestone-2 records have no public_events key: load an empty list.
        const auto publicEventsJson = j.find("public_events");
        if (publicEventsJson != j.end()) {
            if (!publicEventsJson->is_array()) {
                return false;
            }
            for (const auto& entryJson : *publicEventsJson) {
                PublicEventEntry entry;
                if (!hider_seeker::fromJson(entryJson, entry)) {
                    return false;
                }
                parsed.public_events.push_back(std::move(entry));
            }
        }
    } catch (const nlohmann::json::exception&) {
        return false;
    }

    *this = std::move(parsed);
    return true;
}

void appendPublicEvent(MatchRecord& record, const PublicEventEntry& entry) {
    if (record.public_events.size() >= MAX_EVENTS) {
        record.public_events.erase(record.public_events.begin());
    }
    record.public_events.push_back(entry);
}

} // namespace hider_seeker
