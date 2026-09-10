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
        std::vector<std::optional<NodeId>> m_children;
        bool m_is_end = false;

        Node()
            : m_children(RANGE_SIZE)
        {
        }
    };

    std::vector<Node> m_nodes;

    template<typename... Args>
    [[nodiscard]] constexpr NodeId make_node(Args&&... args)
    {
        NodeId id = m_nodes.size();
        m_nodes.emplace_back(std::forward<Args>(args)...);
        return id;
    }

    [[nodiscard]] constexpr NodeId dup_node(NodeId id)
    {
        Node dup = get_node(id);
        return make_node(std::move(dup));
    }

    [[nodiscard]] constexpr NodeId dup_node_or_new(std::optional<NodeId> id)
    {
        if (!id)
            return make_node();

        return dup_node(*id);
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
        return m_nodes.at(id);
    }

    [[nodiscard]] constexpr const Node& get_node(NodeId id) const
    {
        return m_nodes.at(id);
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
        NodeId cur_dst = dup_node_or_new(v.node_id);
        Version out{cur_dst};

        std::optional<NodeId> cur_src = v.node_id;

        for (char c : s) {
            std::size_t mc = map_char(c);

            if (cur_src)
                cur_src = get_node(*cur_src).m_children.at(mc);

            NodeId next = dup_node_or_new(cur_src);
            get_node(cur_dst).m_children.at(mc) = next;
            cur_dst = next;
        }

        get_node(cur_dst).m_is_end = true;
        return out;
    }

    [[nodiscard]] bool contains(Version v, std::string_view s) const
    {
        std::optional<NodeId> cur = v.node_id;

        const auto* it = s.begin();
        while (it != s.end() && cur)
            cur = get_node(*cur).m_children.at(map_char(*it++));

        return it == s.end() && cur && get_node(*cur).m_is_end;
    }
};

#endif
