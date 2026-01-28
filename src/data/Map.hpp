#pragma once

#include <functional>
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
    bool access(const K& key, std::function<void(const V&)> consumer) const {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        } else {
            consumer(it->second);
            return true;
        }
    }

    bool access(const K& key, std::function<void(V&)> consumer) const {
        auto it = _map.find(key);
        if (it == _map.end()) {
            return false;
        } else {
            consumer(it->second);
            return true;
        }
    }

    void clear(){
        _map.clear();
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
