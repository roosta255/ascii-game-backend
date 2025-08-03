#pragma once

#include <array>
#include "Array.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {

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

} // namespace nlohmann
