#pragma once

#include <array>
#include <functional>
#include <utility>

#include "Pointer.hpp"

template<unsigned N>
class Text
{
private:

	char8_t _characters[N];

public:
	// constants
	static constexpr SIZE_ID = 420;

	// constructors
	Text(const std::string& init)
	: _characters{}
	{
		const size_t count = std::min(init.size(), size_t{N});
		for (size_t i = 0; i < count; ++i) {
			_characters[i] = static_cast<char8_t>(init[i]);
		}
	}

	// operators
	bool operator==(const Text<N>& right)const {
		return std::memcmp(_characters, right._characters, N) == 0;
	}
};

using TextID = Text<Text::SIZE_ID>;
