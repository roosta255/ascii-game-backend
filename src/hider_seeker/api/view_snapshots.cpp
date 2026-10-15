#include "view_snapshots.hpp"

namespace hider_seeker {

namespace {

using nlohmann::json;

std::string snapshotFilename(const std::string& matchId, const std::string& audienceKey, uint64_t revision) {
    return matchId + ".view." + audienceKey + ".rev-" + std::to_string(revision);
}

} // namespace

ViewContext viewContextOf(const MatchRecord& record) {
    ViewContext ctx;
    ctx.matchId = record.filename;
    ctx.accounts = record.accounts;
    ctx.publicEvents = record.public_events;
    return ctx;
}

bool writeSnapshotIfAbsent(iStore& store, const std::string& matchId, const std::string& audienceKey,
                           uint64_t revision, const nlohmann::json& view) {
    try {
        const std::string filename = snapshotFilename(matchId, audienceKey, revision);

        // Never touch an existing document - including an unreadable one,
        // which the store itself would not recognize as existing.
        CodeEnum loadError = CODE_SUCCESS;
        json existing;
        if (store.load(filename, loadError, existing)) {
            return true;
        }
        if (loadError != CODE_FILE_NOT_FOUND_ON_DISK) {
            return false;
        }

        const json document = {{"filename", filename}, {"version", 0}, {"view", view}};
        CodeEnum saveError = CODE_SUCCESS;
        return store.save(true, saveError, document);
    } catch (...) {
        return false;
    }
}

bool publishSnapshots(iStore& store, const MatchRecord& record) {
    try {
        const ViewContext ctx = viewContextOf(record);
        const MatchState& state = record.state;
        bool allWritten = writeSnapshotIfAbsent(store, ctx.matchId, "public", state.revision,
                                                buildPublicView(state, ctx));

        // The index is the hider slot named in the audience key.
        for (uint8_t slot = 0; slot < state.hider_count && slot < MAX_HIDERS; ++slot) {
            bool bound = false;
            ctx.accounts.accessConst(slot, [&bound](const std::string& account) { bound = !account.empty(); });
            if (!bound) {
                continue;
            }
            const bool written = writeSnapshotIfAbsent(store, ctx.matchId, "hider-" + std::to_string(slot),
                                                       state.revision, buildHiderView(state, ctx, slot));
            allWritten = allWritten && written;
        }
        return allWritten;
    } catch (...) {
        return false;
    }
}

bool loadSnapshot(iStore& store, const std::string& matchId, const std::string& audienceKey, uint64_t revision,
                  nlohmann::json& outView) {
    try {
        CodeEnum error = CODE_SUCCESS;
        json document;
        if (!store.load(snapshotFilename(matchId, audienceKey, revision), error, document)) {
            return false;
        }
        if (!document.is_object()) {
            return false;
        }
        const auto filename = document.find("filename");
        const auto version = document.find("version");
        const auto view = document.find("view");
        if (filename == document.end() || !filename->is_string()) {
            return false;
        }
        if (version == document.end() || !version->is_number_integer() || version->get<int64_t>() != 0) {
            return false;
        }
        if (view == document.end() || !view->is_object()) {
            return false;
        }
        outView = *view;
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace hider_seeker
