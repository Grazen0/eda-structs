#ifndef EDA_STRUCTS_PERSISTENT_TRIE_HPP
#define EDA_STRUCTS_PERSISTENT_TRIE_HPP

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

template<char BASE = 'a', std::size_t RANGE_SIZE = 'z' - 'a' + 1>
class PersistentTrie {
private:
    using NodeId = std::size_t;

    struct Node {
        std::vector<std::optional<NodeId>> children;
        bool is_end = false;

        Node()
            : children(RANGE_SIZE)
        {
        }
    };

    std::vector<Node> nodes;

    template<typename... Args>
    [[nodiscard]] NodeId make_node(Args&&... args)
    {
        NodeId id = nodes.size();
        nodes.emplace_back(std::forward<Args>(args)...);
        return id;
    }

    [[nodiscard]] static constexpr std::size_t map_char(char c)
    {
        if (c < BASE)
            throw std::out_of_range("character below trie mapping range");

        std::size_t out = c - BASE;
        if (out >= RANGE_SIZE)
            throw std::out_of_range("character over trie mapping range");

        return out;
    }

    [[nodiscard]] constexpr Node& get_node(NodeId id)
    {
        return nodes.at(id);
    }

    [[nodiscard]] constexpr const Node& get_node(NodeId id) const
    {
        return nodes.at(id);
    }

public:
    class Version {
        std::optional<NodeId> node_id;

        explicit Version(std::optional<NodeId> node_id)
            : node_id{node_id}
        {
        }

        friend class PersistentTrie;
    };

    [[nodiscard]] constexpr Version init()
    {
        return Version{std::nullopt};
    }

    [[nodiscard]] Version insert(Version v, std::string_view s)
    {
        Version out{std::nullopt};
        // Note that cur_dst being a pointer assumes the following:
        // 1. out.node_id is never moved somewhere else
        // 2. node children are never reallocated
        std::optional<NodeId>* cur_dst = &out.node_id;
        std::optional<NodeId> cur_src = v.node_id;

        for (char c : s) {
            std::size_t mc = map_char(c);
            if (cur_src) {
                Node cpy = get_node(*cur_src);
                *cur_dst = make_node(std::move(cpy));
            } else {
                *cur_dst = make_node();
            }

            cur_dst = &get_node(**cur_dst).children.at(mc);

            if (cur_src)
                cur_src = get_node(*cur_src).children.at(mc);
        }

        *cur_dst = cur_src ? make_node(get_node(*cur_src)) : make_node();
        get_node(**cur_dst).is_end = true;
        return out;
    }

    [[nodiscard]] bool contains(Version v, std::string_view s) const
    {
        std::optional<NodeId> cur = v.node_id;

        const auto* it = s.begin();
        while (it != s.end() && cur)
            cur = get_node(*cur).children.at(map_char(*it++));

        return it == s.end() && cur.has_value() && get_node(*cur).is_end;
    }
};

#endif
