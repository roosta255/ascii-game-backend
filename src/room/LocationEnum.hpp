#pragma once

#include <ostream>
#include <string>

#define LOCATION_DECL( name ) LOCATION_##name,
enum LocationEnum: int32_t
{
#include "Location.enum"
LOCATION_COUNT
};
#undef LOCATION_DECL

const char* location_to_text(LocationEnum);
bool text_to_location(const std::string& text, LocationEnum& output);

std::ostream& operator<<(std::ostream& os, LocationEnum code);
