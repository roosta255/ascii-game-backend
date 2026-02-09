#pragma once

#include <string>
#include <algorithm>
#include <cstring>

#include <cstdint>
#include <string>
#include <algorithm>
#include <cstring>

template<unsigned N>
class Text
{
private:
    // Move _size to the front to establish 4-byte alignment immediately
    uint32_t _size = 0; 
    char _characters[N] = {}; 

public:
    Text() : _size(0), _characters{} {}

    Text(const std::string& init) : _characters{} {
        // Clamp size to N, and use uint32_t for the limit
        _size = static_cast<uint32_t>(std::min(init.size(), static_cast<size_t>(N)));
        for (uint32_t i = 0; i < _size; ++i) {
            _characters[i] = init[i];
        }
    }

    Text(const char* init) : _characters{}, _size(0) {
        if (init == nullptr) return;
        while (_size < N && init[_size] != '\0') {
            _characters[_size] = init[_size];
            _size++;
        }
    }

    uint32_t size() const { return _size; }
    unsigned capacity() const { return N; }
    bool isEmpty() const { return _size == 0; }

    std::string toString() const {
        return std::string(_characters, _size);
    }

    bool operator==(const Text<N>& right) const {
        if (_size != right._size) return false;
        return std::memcmp(_characters, right._characters, _size) == 0;
    }

    bool operator!=(const Text<N>& right) const {
        return !(*this == right);
    }
};

typedef Text<128> TextID;
typedef Text<512> TextPath;
