#include "view_snapshots.hpp"
#include <string>

namespace asciigame {
namespace hider_seeker {

namespace {

std::string makeSnapshotFilename(const std::string& matchId, const std::string& audienceKey, uint64_t revision) {
    return matchId + ".view." + audienceKey + ".rev-" + std::to_string(revision);
}

} // anonymous namespace

bool publishSnapshots(iStore& store, const MatchRecord& record) {
    // Build ViewContext from the record
    ViewContext ctx;
    ctx.matchId = record.filename;
    ctx.accounts = record.accounts;
    ctx.publicEvents = record.public_events;
    
    // Build public view
    nlohmann::json publicView = buildPublicView(record.state, ctx);
    std::string publicAudienceKey = "public";
    if (!writeSnapshotIfAbsent(store, record.filename, publicAudienceKey, record.state.revision, publicView)) {
        return false;
    }
    
    // Build hider views for each bound slot
    for (int i = 0; i < record.state.hider_count; ++i) {
        if (!record.accounts[i].empty()) {  // Slot is bound
            std::string hiderAudienceKey = "hider-" + std::to_string(i);
            nlohmann::json hiderView = buildHiderView(record.state, ctx, i);
            if (!writeSnapshotIfAbsent(store, record.filename, hiderAudienceKey, record.state.revision, hiderView)) {
                return false;
            }
        }
    }
    
    return true;
}

bool loadSnapshot(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision, nlohmann::json& outView) {
    std::string filename = makeSnapshotFilename(matchId, audienceKey, revision);
    
    // Try to load the document
    std::string content;
    if (!store.load(filename, content)) {
        return false;  // Document doesn't exist
    }
    
    try {
        nlohmann::json doc = nlohmann::json::parse(content);
        
        // Validate document structure
        if (!doc.contains("filename") || !doc.contains("version") || !doc.contains("view")) {
            return false;
        }
        
        if (doc["version"] != 0) {
            return false;  // Unsupported version
        }
        
        if (!doc["view"].is_object()) {
            return false;  // View is not an object
        }
        
        outView = doc["view"];
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;  // Malformed JSON
    }
}

bool writeSnapshotIfAbsent(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision, const nlohmann::json& view) {
    std::string filename = makeSnapshotFilename(matchId, audienceKey, revision);
    
    // Check if document already exists
    std::string existingContent;
    if (store.load(filename, existingContent)) {
        // Document already exists, don't overwrite
        return true;
    }
    
    // Create the document to store
    nlohmann::json doc;
    doc["filename"] = filename;
    doc["version"] = 0;
    doc["view"] = view;
    
    // Save it with isInitial = true (never overwrites existing documents)
    return store.save(filename, doc.dump(), true);
}

} // namespace hider_seeker
} // namespace asciigame