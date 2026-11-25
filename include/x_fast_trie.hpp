#pragma once
#include <algorithm>
#include <bitset>
#include <cassert>
#include <climits>
#include <cstddef>
#include <iostream>
#include <list>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <utility>

struct pair_hash {
    template<class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        return h1 ^ h2;
    }
};

template<std::size_t W>
    requires(1 <= W && W <= ULLONG_WIDTH)
class XFastTrie {
private:
    struct Entry {
        std::bitset<W> m_min;
        std::bitset<W> m_max;

        Entry(std::bitset<W> min, std::bitset<W> max)
            : m_min{std::move(min)},
              m_max{std::move(max)}
        {
        }
    };

    using key_list = std::list<std::bitset<W>>;
    using map_key = std::pair<std::size_t, std::bitset<W>>;

    std::unordered_map<map_key, Entry, pair_hash> m_map{};
    key_list m_list{};
    std::unordered_map<std::bitset<W>, typename key_list::iterator>
        m_list_map{};

    [[nodiscard]] std::optional<std::bitset<W>> sig(std::bitset<W> x) const
    {
        auto it = m_list_map.at(x);
        ++it;
        if (it == m_list.end())
            return std::nullopt;

        return *it;
    }

public:
    [[nodiscard]] constexpr bool empty() const
    {
        return m_list.empty();
    }

    [[nodiscard]] constexpr bool size() const
    {
        return m_list.size();
    }

    [[nodiscard]] std::optional<std::bitset<W>> successor(std::bitset<W> x)
    {
        auto x_ull = x.to_ullong();

        std::size_t lo = 0;
        std::size_t hi = W;

        while (lo < hi) {
            std::size_t mid = std::midpoint(hi, lo);
            if (m_map.contains({mid, x_ull >> (W - mid)}))
                lo = mid;
            else
                hi = mid - 1;
        }

        std::size_t l = lo;
        auto p = x_ull >> (W - l);

        if (l == W)
            return sig(x);

        if (!x.test(W - 1 - l))
            return m_map.at({l + 1, (p << 1) | 1}).m_min;

        std::bitset<W> y = m_map.at({l + 1, p << 1}).m_max;
        return sig(y);
    }

    bool insert(std::bitset<W> x)
    {
        if (m_list_map.contains(x))
            return false;

        auto x_ull = x.to_ullong();

        auto succ = successor(x);
        auto dest = succ ? m_list_map.at(*succ) : m_list.end();

        auto it = m_list.emplace(dest, x);
        m_list_map.emplace(x, it);

        for (std::size_t l = 0; l <= W; ++l) {
            map_key key{l, l == 0 ? 0 : (x_ull >> (W - l))};
            auto it = m_map.find(key);
            if (it == m_map.end()) {
                m_map.emplace(key, Entry{x, x});
            } else {
                it->second.m_min =
                    std::min(it->second.m_min.to_ullong(), x_ull);
                it->second.m_max =
                    std::max(it->second.m_max.to_ullong(), x_ull);
            }
        }

        return true;
    }
};
