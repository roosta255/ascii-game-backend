#pragma once

#include <array>
#include "Array.hpp"
#include "TraitBits.hpp"
#include "TraitFlyweight.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {

// Serializes TraitBits as an array of trait name strings for the set bits.
// Unknown names on deserialization are silently skipped for forward compatibility.
template<>
struct adl_serializer<TraitBits> {
    static void to_json(json& j, const TraitBits& bits) {
        j = json::array();
        for (int i = 0; i < (int)TRAIT_COUNT; ++i) {
            if (bits[i].orElse(false)) {
                TraitFlyweight::getFlyweights().accessConst(i, [&](const TraitFlyweight& flyweight) {
                    j.push_back(flyweight.name);
                });
            }
        }
    }

    static void from_json(const json& j, TraitBits& bits) {
        if (!j.is_array())
            throw json::type_error::create(302, "Expected array of trait names", j);
        bits = TraitBits{};
        for (const auto& item : j) {
            TraitEnum traitEnum;
            if (TraitFlyweight::indexByString(item.get<std::string>(), traitEnum))
                bits.setIndexOn((Bitline)traitEnum);
        }
    }
};

template<typename T, unsigned N>
struct adl_serializer<Array<T, N>> {
    static void to_json(json& j, const Array<T, N>& view) {
        view.access([&](const std::array<T, N>& data){
            j = data; // Serializes as a JSON array
        });
    }

    static void from_json(const json& j, Array<T, N>& view) {
        if (!j.is_array() || j.size() != N) {
            throw json::type_error::create(302, "Expected array of size " + std::to_string(N), j);
        }
        for (unsigned i = 0; i < N; ++i) {
            view.access(i, [&](T& data){
                data = j.at(i).get<T>();
            });
        }
    }
};

template<typename T>
struct adl_serializer<Rack<T>> {
    static void to_json(json& j, const Rack<T>& view) {
        const auto size = view.size();
        j = json::array();

        for (const auto& e: view) {
            j.push_back(e);
        }
    }

    static void from_json(const json& j, Rack<T>& view) {
        if (!j.is_array()) {
            throw json::type_error::create(302, "Expected array", j);
        }

        if (j.size() > view.size()) {
            throw json::type_error::create(
                302,
                "JSON array too large for Rack capacity",
                j
            );
        }

        for (size_t i = 0; i < j.size(); ++i) {
            view.access(static_cast<int>(i), [&](T& data) {
                data = j.at(i).get<T>();
            });
        }
    }
};

} // namespace nlohmann
