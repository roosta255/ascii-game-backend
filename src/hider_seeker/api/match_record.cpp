#include "match_record.hpp"
#include "../domain/serialization.hpp"
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace hider_seeker {

nlohmann::json MatchRecord::toJson() const {
    nlohmann::json j;
    j["filename"] = filename;
    j["version"] = version;
    j["host"] = host;
    j["state"] = state;
    
    // Convert accounts array to JSON
    j["accounts"] = accounts;
    
    j["next_move_id"] = next_move_id;
    
    // Convert idempotency array to JSON
    j["idempotency"] = idempotency;
    
    // Convert moves array to JSON
    j["moves"] = moves;
    
    // Convert request_log array to JSON
    j["request_log"] = request_log;
    
    return j;
}

bool MatchRecord::fromJson(const nlohmann::json& j) {
    try {
        filename = j.value("filename", "");
        version = j.value("version", 0);
        host = j.value("host", "");
        state = j.value("state", "");
        
        // Parse accounts array
        if (j.contains("accounts")) {
            accounts = j["accounts"].get<std::vector<nlohmann::json>>();
        } else {
            accounts.clear();
        }
        
        next_move_id = j.value("next_move_id", 1);
        
        // Parse idempotency array
        if (j.contains("idempotency")) {
            idempotency = j["idempotency"].get<std::vector<IdempotencyEntry>>();
        } else {
            idempotency.clear();
        }
        
        // Parse moves array
        if (j.contains("moves")) {
            moves = j["moves"].get<std::vector<MoveOwnershipEntry>>();
        } else {
            moves.clear();
        }
        
        // Parse request_log array
        if (j.contains("request_log")) {
            request_log = j["request_log"].get<std::vector<RequestLogEntry>>();
        } else {
            request_log.clear();
        }
        
        // Validate state field is valid hex and can be decoded
        if (!state.empty()) {
            // Check if it's valid hex
            bool isValidHex = true;
            for (char c : state) {
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                    isValidHex = false;
                    break;
                }
            }
            
            if (!isValidHex) {
                return false;
            }
            
            // Try to decode the hex string
            std::vector<uint8_t> bytes(state.length() / 2);
            for (size_t i = 0; i < state.length(); i += 2) {
                std::string byteString = state.substr(i, 2);
                try {
                    bytes[i/2] = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
                } catch (...) {
                    return false;
                }
            }
            
            // Try to load the match state - this will fail if it's schema version 1
            MatchState tempState{};
            if (!loadMatchState(bytes.data(), bytes.size(), tempState)) {
                return false;
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace hider_seeker