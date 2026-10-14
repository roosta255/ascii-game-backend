#include "serialization.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

namespace hider_seeker {

namespace {
    constexpr uint32_t CURRENT_SCHEMA_VERSION = 2;

    void writeUint8(std::vector<uint8_t>& out, uint8_t value) {
        out.push_back(value);
    }

    void writeUint16(std::vector<uint8_t>& out, uint16_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    void writeUint32(std::vector<uint8_t>& out, uint32_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    void writeUint64(std::vector<uint8_t>& out, uint64_t value) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
        out.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    }

    uint8_t readUint8(const std::vector<uint8_t>& in, size_t& pos) {
        return in[pos++];
    }

    uint16_t readUint16(const std::vector<uint8_t>& in, size_t& pos) {
        uint16_t value = 0;
        value |= static_cast<uint16_t>(in[pos++]);
        value |= static_cast<uint16_t>(in[pos++] << 8);
        return value;
    }

    uint32_t readUint32(const std::vector<uint8_t>& in, size_t& pos) {
        uint32_t value = 0;
        value |= static_cast<uint32_t>(in[pos++]);
        value |= static_cast<uint32_t>(in[pos++] << 8);
        value |= static_cast<uint32_t>(in[pos++] << 16);
        value |= static_cast<uint32_t>(in[pos++] << 24);
        return value;
    }

    uint64_t readUint64(const std::vector<uint8_t>& in, size_t& pos) {
        uint64_t value = 0;
        value |= static_cast<uint64_t>(in[pos++]);
        value |= static_cast<uint64_t>(in[pos++]) << 8;
        value |= static_cast<uint64_t>(in[pos++]) << 16;
        value |= static_cast<uint64_t>(in[pos++]) << 24;
        value |= static_cast<uint64_t>(in[pos++]) << 32;
        value |= static_cast<uint64_t>(in[pos++]) << 40;
        value |= static_cast<uint64_t>(in[pos++]) << 48;
        value |= static_cast<uint64_t>(in[pos++]) << 56;
        return value;
    }

    void writeMetricFrame(std::vector<uint8_t>& out, const MetricFrame& frame) {
        writeUint8(out, frame.cpu);
        writeUint8(out, frame.memory);
        writeUint8(out, frame.disk);
        writeUint8(out, frame.network);
        writeUint8(out, frame.heat);
        writeUint8(out, frame.errors);
        writeUint8(out, frame.users);
    }

    MetricFrame readMetricFrame(const std::vector<uint8_t>& in, size_t& pos) {
        MetricFrame frame;
        frame.cpu = readUint8(in, pos);
        frame.memory = readUint8(in, pos);
        frame.disk = readUint8(in, pos);
        frame.network = readUint8(in, pos);
        frame.heat = readUint8(in, pos);
        frame.errors = readUint8(in, pos);
        frame.users = readUint8(in, pos);
        return frame;
    }

    void writeServerState(std::vector<uint8_t>& out, const ServerState& state) {
        writeUint8(out, state.active);
        writeUint8(out, state.owner_hider_id);
        writeUint8(out, state.public_flags);
        writeUint8(out, state.trace_strength);
        writeUint8(out, state.trace_age);
        writeMetricFrame(out, state.current);
        for (int i = 0; i < HISTORY_TICKS; ++i) {
            writeMetricFrame(out, state.history[i]);
        }
        writeUint8(out, state.history_head);
    }

    ServerState readServerState(const std::vector<uint8_t>& in, size_t& pos) {
        ServerState state;
        state.active = readUint8(in, pos);
        state.owner_hider_id = readUint8(in, pos);
        state.public_flags = readUint8(in, pos);
        state.trace_strength = readUint8(in, pos);
        state.trace_age = readUint8(in, pos);
        state.current = readMetricFrame(in, pos);
        for (int i = 0; i < HISTORY_TICKS; ++i) {
            state.history[i] = readMetricFrame(in, pos);
        }
        state.history_head = readUint8(in, pos);
        return state;
    }

    void writeHiderState(std::vector<uint8_t>& out, const HiderState& state) {
        writeUint8(out, state.active);
        writeUint8(out, state.status);
        writeUint8(out, state.action_points);
        writeUint8(out, state.server_id);
        for (int i = 0; i < RESOURCE_TYPE_COUNT; ++i) {
            writeUint16(out, state.resources[i]);
        }
        for (int i = 0; i < MAX_GOALS; ++i) {
            writeUint8(out, state.goal_progress[i]);
        }
    }

    HiderState readHiderState(const std::vector<uint8_t>& in, size_t& pos) {
        HiderState state;
        state.active = readUint8(in, pos);
        state.status = readUint8(in, pos);
        state.action_points = readUint8(in, pos);
        state.server_id = readUint8(in, pos);
        for (int i = 0; i < RESOURCE_TYPE_COUNT; ++i) {
            state.resources[i] = readUint16(in, pos);
        }
        for (int i = 0; i < MAX_GOALS; ++i) {
            state.goal_progress[i] = readUint8(in, pos);
        }
        return state;
    }

    void writeSeekerState(std::vector<uint8_t>& out, const SeekerState& state) {
        writeUint8(out, state.connection_tokens);
        writeUint8(out, state.max_tokens);
        writeUint8(out, state.heat);
        for (int i = 0; i < MAX_SERVERS; ++i) {
            writeUint8(out, state.belief[i]);
        }
        for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
            writeUint8(out, state.target_ids[i]);
        }
        writeUint8(out, state.target_count);
        writeUint8(out, state.target_head);
        for (int i = 0; i < MAX_SERVERS; ++i) {
            writeUint8(out, state.last_probe_tick[i]);
        }
    }

    SeekerState readSeekerState(const std::vector<uint8_t>& in, size_t& pos) {
        SeekerState state;
        state.connection_tokens = readUint8(in, pos);
        state.max_tokens = readUint8(in, pos);
        state.heat = readUint8(in, pos);
        for (int i = 0; i < MAX_SERVERS; ++i) {
            state.belief[i] = readUint8(in, pos);
        }
        for (int i = 0; i < MAX_SEEKER_TARGETS; ++i) {
            state.target_ids[i] = readUint8(in, pos);
        }
        state.target_count = readUint8(in, pos);
        state.target_head = readUint8(in, pos);
        for (int i = 0; i < MAX_SERVERS; ++i) {
            state.last_probe_tick[i] = readUint8(in, pos);
        }
        return state;
    }

    void writeRequirement(std::vector<uint8_t>& out, const Requirement& req) {
        writeUint8(out, req.type);
        writeUint8(out, req.subject_id);
        writeUint16(out, req.amount);
        writeUint16(out, req.threshold);
    }

    Requirement readRequirement(const std::vector<uint8_t>& in, size_t& pos) {
        Requirement req;
        req.type = readUint8(in, pos);
        req.subject_id = readUint8(in, pos);
        req.amount = readUint16(in, pos);
        req.threshold = readUint16(in, pos);
        return req;
    }

    void writeGoalState(std::vector<uint8_t>& out, const GoalState& state) {
        writeUint8(out, state.active);
        writeUint8(out, state.goal_id);
        writeUint8(out, state.status);
        writeUint8(out, state.reward_type);
        writeUint16(out, state.reward_amount);
        writeUint8(out, state.requirement_count);
        for (int i = 0; i < MAX_REQUIREMENTS_PER_GOAL; ++i) {
            writeRequirement(out, state.requirements[i]);
        }
        writeUint8(out, state.completion_action);
        writeUint16(out, state.deadline_tick);
    }

    GoalState readGoalState(const std::vector<uint8_t>& in, size_t& pos) {
        GoalState state;
        state.active = readUint8(in, pos);
        state.goal_id = readUint8(in, pos);
        state.status = readUint8(in, pos);
        state.reward_type = readUint8(in, pos);
        state.reward_amount = readUint16(in, pos);
        state.requirement_count = readUint8(in, pos);
        for (int i = 0; i < MAX_REQUIREMENTS_PER_GOAL; ++i) {
            state.requirements[i] = readRequirement(in, pos);
        }
        state.completion_action = readUint8(in, pos);
        state.deadline_tick = readUint16(in, pos);
        return state;
    }

    void writeEvent(std::vector<uint8_t>& out, const Event& event) {
        writeUint8(out, event.type);
        writeUint32(out, event.tick);
    }

    Event readEvent(const std::vector<uint8_t>& in, size_t& pos) {
        Event event;
        event.type = readUint8(in, pos);
        event.tick = readUint32(in, pos);
        return event;
    }

    void writePendingMove(std::vector<uint8_t>& out, const PendingMove& move) {
        writeUint32(out, move.move_id);
        writeUint32(out, move.submit_seq);
        writeUint8(out, move.hider_id);
        writeUint8(out, move.action_type);
        writeUint8(out, move.server_id);
        writeUint8(out, move.target_id);
        writeUint8(out, move.server_mode);
    }

    PendingMove readPendingMove(const std::vector<uint8_t>& in, size_t& pos) {
        PendingMove move;
        move.move_id = readUint32(in, pos);
        move.submit_seq = readUint32(in, pos);
        move.hider_id = readUint8(in, pos);
        move.action_type = readUint8(in, pos);
        move.server_id = readUint8(in, pos);
        move.target_id = readUint8(in, pos);
        move.server_mode = readUint8(in, pos);
        return move;
    }
} // anonymous namespace

void saveMatchState(const MatchState& state, std::vector<uint8_t>& out) {
    // Leading marker: the buffer layout version this code writes (schema
    // version 2 adds the pending-move ring). state.schema_version itself is
    // stored as an ordinary field right after it.
    writeUint32(out, CURRENT_SCHEMA_VERSION);
    writeUint32(out, state.schema_version);
    writeUint32(out, state.ruleset_version);
    writeUint64(out, state.revision);
    writeUint32(out, state.tick);
    writeUint64(out, state.rng_state);
    writeUint8(out, state.phase);
    writeUint8(out, state.hider_count);
    writeUint8(out, state.server_count);
    
    for (int i = 0; i < MAX_HIDERS; ++i) {
        writeHiderState(out, state.hiders[i]);
    }
    
    for (int i = 0; i < MAX_SERVERS; ++i) {
        writeServerState(out, state.servers[i]);
    }
    
    writeSeekerState(out, state.seeker);
    
    for (int i = 0; i < MAX_GOALS; ++i) {
        writeGoalState(out, state.goals[i]);
    }
    
    for (int i = 0; i < MAX_EVENTS; ++i) {
        writeEvent(out, state.events[i]);
    }
    
    writeUint8(out, state.event_head);
    writeUint8(out, state.event_count);
    
    // Write pending move ring buffer
    for (const auto& pending_move : state.pending) {
        writePendingMove(out, pending_move);
    }
    writeUint8(out, state.pending_head);
    writeUint8(out, state.pending_count);
    writeUint32(out, state.next_submit_seq);
}

bool loadMatchState(const std::vector<uint8_t>& in, MatchState& state) {
    if (in.empty()) {
        return false;
    }
    
    // Check that we have at least 4 bytes for the schema version marker
    if (in.size() < 4) {
        return false;
    }
    
    size_t pos = 0;
    uint32_t buffer_schema_version = readUint32(in, pos);

    // Check if this is a supported schema version (version-1 buffers, which
    // have no pending-move ring, are rejected like any other mismatch)
    if (buffer_schema_version != CURRENT_SCHEMA_VERSION) {
        return false;
    }

    // Bounds-check the fixed header fields before reading any of them --
    // schema_version(4) + ruleset_version(4) + revision(8) + tick(4)
    // + rng_state(8) + phase(1) + hider_count(1) + server_count(1) = 31 bytes.
    constexpr size_t kHeaderRemainderSize = 4 + 4 + 8 + 4 + 8 + 1 + 1 + 1;
    if (pos + kHeaderRemainderSize > in.size()) {
        return false;
    }

    // Parse into a local copy and only commit it to `state` once every
    // check has passed, so a rejected buffer never partially populates state.
    MatchState loaded{};
    loaded.schema_version = readUint32(in, pos);
    loaded.ruleset_version = readUint32(in, pos);
    loaded.revision = readUint64(in, pos);
    loaded.tick = readUint32(in, pos);
    loaded.rng_state = readUint64(in, pos);
    loaded.phase = readUint8(in, pos);
    loaded.hider_count = readUint8(in, pos);
    loaded.server_count = readUint8(in, pos);
    
    // Bounds-check the remaining bulk sections before reading any of them
    // -- each read*() helper below indexes in[] unchecked, so a
    // truncated-but-non-empty, correctly-marked buffer must be rejected
    // here rather than relied on to fail safely on its own.
    constexpr size_t kHiderStateSize = 4 + RESOURCE_TYPE_COUNT * 2 + MAX_GOALS;
    constexpr size_t kServerStateSize = 5 + 7 + HISTORY_TICKS * 7 + 1;
    constexpr size_t kSeekerStateSize = 5 + 2 * MAX_SERVERS + MAX_SEEKER_TARGETS;
    constexpr size_t kGoalStateSize = 10 + MAX_REQUIREMENTS_PER_GOAL * 6;
    constexpr size_t kEventSize = 5;
    constexpr size_t kPendingMoveSize = 4 + 4 + 1 + 1 + 1 + 1 + 1; // move_id, submit_seq, hider_id, action_type, server_id, target_id, server_mode
    constexpr size_t kExpectedBulkSize =
        MAX_HIDERS * kHiderStateSize
        + MAX_SERVERS * kServerStateSize
        + kSeekerStateSize
        + MAX_GOALS * kGoalStateSize
        + MAX_EVENTS * kEventSize
        + 2 // event_head + event_count
        + MAX_PENDING_MOVES * kPendingMoveSize // pending moves
        + 1 + 1 + 4; // pending_head, pending_count, next_submit_seq
    if (pos + kExpectedBulkSize > in.size()) {
        return false;
    }

    // Read hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        loaded.hiders[i] = readHiderState(in, pos);
    }

    // Read servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        loaded.servers[i] = readServerState(in, pos);
    }

    // Read seeker
    loaded.seeker = readSeekerState(in, pos);

    // Read goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        loaded.goals[i] = readGoalState(in, pos);
    }

    // Read events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        loaded.events[i] = readEvent(in, pos);
    }

    loaded.event_head = readUint8(in, pos);
    loaded.event_count = readUint8(in, pos);

    // Read pending move ring buffer
    for (auto& pending_move : loaded.pending) {
        pending_move = readPendingMove(in, pos);
    }

    loaded.pending_head = readUint8(in, pos);
    loaded.pending_count = readUint8(in, pos);
    loaded.next_submit_seq = readUint32(in, pos);

    // Validate pending fields
    if (loaded.pending_count > MAX_PENDING_MOVES) {
        return false;
    }
    if (loaded.pending_head >= MAX_PENDING_MOVES) {
        return false;
    }

    state = loaded;
    return true;
}

} // namespace hider_seeker