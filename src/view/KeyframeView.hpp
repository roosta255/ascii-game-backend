#pragma once

#include "adl_serializer.hpp"
#include "Keyframe.hpp"
#include "KeyframeFlyweight.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include "Timestamp.hpp"
#include <vector>

struct KeyframeView
{
    std::string animation = "UNPARSED_ANIMATION";
    long t0 = 0;
    long t1 = 0;
    int room0 = 0;
    std::vector<int> data;

    inline KeyframeView() = default;

    inline KeyframeView(const Keyframe& model)
    {
        this->animation = animation_to_text(model.animation);
        this->t0 = (long)model.t0;
        this->t1 = (long)model.t1;
        this->room0 = model.room0;

        data.reserve(Keyframe::DATA_ARRAY_SIZE);
        for (const auto& d: model.data) {
            this->data.push_back(d);
        }

        // role
        KeyframeFlyweight::getFlyweights().accessConst(model.animation, [&](const KeyframeFlyweight& flyweight) {
            this->animation = flyweight.name;
        });
    }

    inline operator Keyframe() const 
    {
        Keyframe model{
            .t0 = Timestamp::buildTimestamp(this->t0),
            .t1 = Timestamp::buildTimestamp(this->t1),
            .room0 = this->room0
        };

        int i = 0;
        for (auto& d: model.data) {
            if (i < this->data.size()) {
                d = this->data[i];
            }
            i++;
        }

        KeyframeFlyweight::indexByString(this->animation, model.animation);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyframeView, animation, t0, t1, room0, data)
