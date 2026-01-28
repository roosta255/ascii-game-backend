#pragma once

#include <ostream>
#include <string>

#define CHANNEL_DECL( name ) CHANNEL_##name,
enum ChannelEnum: int32_t
{
#include "Channel.enum"
CHANNEL_COUNT
};
#undef CHANNEL_DECL

const char* channel_to_text(ChannelEnum);
bool text_to_channel(const std::string& text, ChannelEnum& output);

std::ostream& operator<<(std::ostream& os, ChannelEnum code);
