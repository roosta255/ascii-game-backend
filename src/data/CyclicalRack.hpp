#pragma once
#include <vector>
#include <functional>

template<typename T>
class CyclicalRack {
    T* buffer_start; // The beginning of the allocated memory
    T* buffer_end;   // The end of the allocated memory (capacity)
    T* head;         // Where the "first" element is
    size_t _size;    // How many elements are currently in the rack

public:
    // Basic Constructor
    CyclicalRack(T* start, size_t capacity) 
        : buffer_start(start), buffer_end(start + capacity), head(start), _size(0) {}

    // Maps a logical index (0 to size-1) to the actual memory address
    T& operator[](size_t index) {
        return *(buffer_start + ((head - buffer_start) + index) % capacity());
    }

    const T& operator[](size_t index) const {
        return *(buffer_start + ((head - buffer_start) + index) % capacity());
    }

    size_t size() const { return _size; }
    size_t capacity() const { return buffer_end - buffer_start; }
    bool isFull() const { return _size == capacity(); }

    void push_back(const T& value) {
        if (isFull()) {
            // Overwrite the oldest data (the current head)
            *head = value;
            head = increment(head);
        } else {
            // Place at the end of the current occupied range
            T* tail = buffer_start + ((head - buffer_start) + _size) % capacity();
            *tail = value;
            _size++;
        }
    }

    // A robust foreach that handles the "looping" logic internally
    void foreach(std::function<void(T&)> func) {
        for (size_t i = 0; i < _size; ++i) {
            func((*this)[i]);
        }
    }

    // --- Iterator Logic ---
    // To support range-based for loops: for(auto& x : rack)
    struct Iterator {
        CyclicalRack& parent;
        size_t index;

        T& operator*() { return parent[index]; }
        Iterator& operator++() { index++; return *this; }
        bool operator!=(const Iterator& other) const { return index != other.index; }
    };

    Iterator begin() { return Iterator{*this, 0}; }
    Iterator end() { return Iterator{*this, _size}; }

private:
    T* increment(T* ptr) {
        ptr++;
        if (ptr == buffer_end) return buffer_start;
        return ptr;
    }
};