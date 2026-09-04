#pragma once

#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Maybe.hpp"

// A small associative container with a deliberately narrow, crash-proof surface.
//
// The backing std::unordered_map invalidates iterators and element references on rehash
// (which any insert may trigger), on erase of the referenced element, and wholesale on
// clear(). To make misuse impossible rather than merely discouraged, this wrapper never
// exposes an iterator or a reference into the storage:
//
//   - reads (get / getOr) return a COPY of the value;
//   - the one in-place mutator (update) hands the callback a DETACHED COPY and writes the
//     result back afterwards, so the callback may freely touch this Map;
//   - iteration (forEach) snapshots the keys up front and re-looks-up each one, handing the
//     callback a const reference to a private copy of the value.
//
// Nothing a caller receives can outlive the Map's next mutation, so no call sequence can
// dangle. The price is value copies; for the scalar keys and small values this project uses
// with Map, that is negligible, and correctness beats the copy.

template<typename K, typename V>
class Map
{
private:
    std::unordered_map<K, V> _map;

public:
    Map() {}
    Map(const K& key, const V& value) { _map[key] = value; }

    // ---- reads: always a copy, never a reference into storage --------------------------

    // The value at `key`, or an empty Maybe if absent.
    Maybe<V> get(const K& key) const {
        auto it = _map.find(key);
        if (it == _map.end()) return Maybe<V>::empty();
        return Maybe<V>(it->second);
    }

    // The value at `key`, or `fallback` if absent. Returns by value - the result is never
    // aliased to storage nor to `fallback`.
    V getOr(const K& key, const V& fallback) const {
        auto it = _map.find(key);
        if (it == _map.end()) return fallback;
        return it->second;
    }

    bool containsKey(const K& key) const { return _map.find(key) != _map.end(); }
    std::size_t size() const { return _map.size(); }
    bool empty() const { return _map.empty(); }

    // ---- writes -----------------------------------------------------------------------

    void set(const K& key, const V& value) { _map[key] = value; }
    bool erase(const K& key) { return _map.erase(key) != 0; }
    void clear() { _map.clear(); }

    // Read-modify-write of a single entry. `fn` receives a mutable COPY of the current
    // value; whatever it leaves there is written back, but only if `key` still exists once
    // `fn` returns. Returns false (no write-back) if the key is absent on entry or was
    // removed by `fn`.
    //
    // Safe against `fn` calling set/erase/clear/update on this Map - those act on storage
    // while `fn` only ever holds the detached copy. If `fn` also writes this same key, this
    // write-back is applied last (last-writer-wins); that is a caller logic question, never
    // a crash.
    template<typename F>
    bool update(const K& key, F&& fn) {
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        V working = it->second;
        fn(working);
        auto after = _map.find(key);
        if (after == _map.end()) return false;
        after->second = std::move(working);
        return true;
    }

    // ---- iteration -------------------------------------------------------------------
    //
    // Visits each key present at entry at most once, in unspecified order. `fn` is called
    // as fn(const K&, const V&) where the value is a reference to a private copy - so `fn`
    // may freely set/erase/clear/rebuild this Map without corrupting the walk:
    //   - a key removed before it is reached is silently skipped;
    //   - keys inserted during the walk are NOT visited (the key set is fixed at entry);
    //   - to change an entry, call set(key, ...) from inside `fn`.
    // If `fn` returns bool, yielding false stops iteration early.
    //
    // Cost: one std::vector<K> plus N lookups. For a nested Map<K, Map<...>>, this guards
    // the inner walk only - snapshot any inner values you need into a local before the
    // outer accessor returns if the outer entry itself can be erased.
    template<typename F>
    void forEach(F&& fn) const {
        std::vector<K> keys;
        keys.reserve(_map.size());
        for (const auto& entry : _map) keys.push_back(entry.first);
        for (const K& key : keys) {
            auto it = _map.find(key);
            if (it == _map.end()) continue;
            const V value = it->second;
            if constexpr (std::is_same_v<std::invoke_result_t<F&, const K&, const V&>, bool>) {
                if (!fn(key, value)) return;
            } else {
                fn(key, value);
            }
        }
    }
};
