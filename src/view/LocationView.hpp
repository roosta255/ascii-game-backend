#pragma once

#include "adl_serializer.hpp"
#include "Location.hpp"
#include <string>

struct LocationView
{
    std::string type = "UNPARSED_LOCATION";
    int roomId = 0;
    std::string channel = "CHANNEL_CORPOREAL";
    int data = 0;

    inline LocationView() = default;

    inline LocationView(const Location& model): type(location_to_text(model.type)), roomId(model.roomId), channel(channel_to_text(model.channel)), data(model.data)
    {}

    inline operator Location() const
    {
        Location model{
            .roomId = roomId,
            .data = data
        };

        text_to_location(type, model.type);
        text_to_channel(channel, model.channel);
        return model;
    }
};

// Reflection-based JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LocationView, type, roomId, channel, data)
