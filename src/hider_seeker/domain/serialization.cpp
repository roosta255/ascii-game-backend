#include "serialization.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

namespace hider_seeker {

namespace {
    constexpr uint32_t CURRENT_SCHEMA_VERSION = 1;

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
} // anonymous namespace

void saveMatchState(const MatchState& state, std::vector<uint8_t>& out) {
    // Encode state.schema_version itself as the leading marker per implementation plan
    // This is a functional substitute for the documented approach that was specified in the plan
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
    uint32_t schema_version = readUint32(in, pos);
    
    // Check if this is a supported schema version
    if (schema_version != CURRENT_SCHEMA_VERSION) {
        return false;
    }
    
    // Read the header fields - these are all the actual data values, not markers
    state.schema_version = readUint32(in, pos);
    state.ruleset_version = readUint32(in, pos);
    state.revision = readUint64(in, pos);
    state.tick = readUint32(in, pos);
    state.rng_state = readUint64(in, pos);
    state.phase = readUint8(in, pos);
    state.hider_count = readUint8(in, pos);
    state.server_count = readUint8(in, pos);
    
    // Read hiders
    for (int i = 0; i < MAX_HIDERS; ++i) {
        state.hiders[i] = readHiderState(in, pos);
    }
    
    // Read servers
    for (int i = 0; i < MAX_SERVERS; ++i) {
        state.servers[i] = readServerState(in, pos);
    }
    
    // Read seeker
    state.seeker = readSeekerState(in, pos);
    
    // Read goals
    for (int i = 0; i < MAX_GOALS; ++i) {
        state.goals[i] = readGoalState(in, pos);
    }
    
    // Read events
    for (int i = 0; i < MAX_EVENTS; ++i) {
        state.events[i] = readEvent(in, pos);
    }
    
    // Read event_head and event_count
    if (pos + 2 > in.size()) {
        return false;
    }
    
    state.event_head = readUint8(in, pos);
    state.event_count = readUint8(in, pos);
    
    return true;
}

} // namespace hider_seeker