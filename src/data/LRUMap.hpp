#pragma once

#include <unordered_map>
#include <list>
#include <functional>

template <typename K, typename V>
class LRUMap {
private:
    size_t _capacity;
    // Stores the actual data: {Key, Value}
    std::list<std::pair<K, V>> _order; 
    // Stores iterator to the list node for O(1) access
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> _map;

    void touch(typename std::list<std::pair<K, V>>::iterator it) {
        // Moves the existing node to the front of the list (most recent)
        _order.splice(_order.begin(), _order, it);
    }

public:
    LRUMap(size_t capacity = 100) : _capacity(capacity) {}

    // Required Interface
    bool access(const K& key, std::function<void(V&)> consumer) {
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        touch(it->second);
        consumer(it->second->second);
        return true;
    }

    bool accessConst(const K& key, std::function<void(const V&)> consumer) const {
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        // Note: const access usually shouldn't modify order, 
        // but if you want LRU to update on reads, this needs 'mutable' members.
        consumer(it->second->second);
        return true;
    }

    void set(const K& key, const V& value) {
        auto it = _map.find(key);
        if (it != _map.end()) {
            // Update existing
            it->second->second = value;
            touch(it->second);
            return;
        }

        // Add new
        _order.emplace_front(key, value);
        _map[key] = _order.begin();

        // Evict if over capacity
        if (_map.size() > _capacity) {
            K lastKey = _order.back().first;
            _map.erase(lastKey);
            _order.pop_back();
        }
    }

    // Standard Map-like methods
    auto begin() const { return _order.begin(); }
    auto end() const { return _order.end(); }
    
    bool containsKey(const K& key) const { return _map.find(key) != _map.end(); }
    
    void erase(const K& key) {
        auto it = _map.find(key);
        if (it != _map.end()) {
            _order.erase(it->second);
            _map.erase(it);
        }
    }

    void clear() {
        _map.clear();
        _order.clear();
    }

    const V& getOrDefault(const K& key, const V& option) {
        auto it = _map.find(key);
        if (it == _map.end()) return option;
        touch(it->second);
        return it->second->second;
    }

    // Assuming your custom Maybe/Pointer types exist:
    Maybe<V> getMaybe(const K& key) {
        auto it = _map.find(key);
        if (it == _map.end()) return Maybe<V>::empty();
        touch(it->second);
        return Maybe<V>(it->second->second);
    }

    Pointer<V> getPointer(const K& key) {
        auto it = _map.find(key);
        if (it == _map.end()) return Pointer<V>::empty();
        touch(it->second);
        return Pointer<V>(it->second->second);
    }
};