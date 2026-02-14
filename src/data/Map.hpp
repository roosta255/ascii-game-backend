#pragma once

#include <functional>
#include "Maybe.hpp"
#include "Pointer.hpp"
#include <unordered_map>

template<typename K, typename V>
class Map
{
private:
    // members
    std::unordered_map<K, V> _map;
public:
    // constructors
    Map(){}
    Map(const K& key, const V& value){
        _map[key] = value;
    }

    // functions

    bool access(const K& key, std::function<void(V&)> consumer) {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        } else {
            consumer(it->second);
            return true;
        }
    }

    bool accessConst(const K& key, std::function<void(const V&)> consumer) const {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        } else {
            consumer(it->second);
            return true;
        }
    }

    auto begin() const { return _map.begin(); }
    auto end() const { return _map.end(); }

    Maybe<V> getMaybe(const K& key) const {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return Maybe<V>::empty();
        } else {
            return Maybe<V>(it->second);
        }
    }

    Pointer<V> getPointer(const K& key) {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return Pointer<V>::empty();
        } else {
            return Pointer<V>(it->second);
        }
    }

    void clear(){ _map.clear(); }

    bool containsKey(const K& key) { return _map.find(key) != _map.end(); }

    void erase(const K& key){
        _map.erase(key);
    }

    const V& getOrDefault(const K& key, const V& option)const{
        auto it = _map.find(key);
        if (it == _map.end()) {
            return option;
        } else {
            return it->second;
        }
    }

    void set(const K& key, const V& value) {
        _map[key] = value;
    }
};
