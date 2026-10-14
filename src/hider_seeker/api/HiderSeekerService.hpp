#pragma once

#include "match_record.hpp"
#include "../store/iStore.hpp"
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace hider_seeker {

struct ApiResponse {
    int status;
    nlohmann::json body;
};

class HiderSeekerService {
public:
    explicit HiderSeekerService(iStore& store);
    
    ApiResponse createMatch(const std::string& body);
    ApiResponse joinMatch(const std::string& matchId, const std::string& body);
    ApiResponse startMatch(const std::string& matchId, const std::string& body);
    ApiResponse getMatch(const std::string& matchId);
    ApiResponse submitMove(const std::string& matchId, const std::string& body);
    ApiResponse cancelMove(const std::string& matchId, const std::string& intentId, const std::string& body);

private:
    iStore& store_;
    
    std::map<std::string, std::unique_ptr<std::mutex>> matchMutexes_;
    std::mutex matchMutexesGuard_;
};

} // namespace hider_seeker