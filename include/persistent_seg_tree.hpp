#ifndef EDA_STRUCTS_PERSISTENT_SEG_TREE_HPP
#define EDA_STRUCTS_PERSISTENT_SEG_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

template<typename T, typename Operator>
class PersistentSegTree {
private:
    using NodeId = std::size_t;

public:
    class Version {
    private:
        explicit Version(NodeId node_id)
            : m_root_id{node_id}
        {
        }

        std::size_t m_root_id;

        friend class PersistentSegTree;
    };

private:
    static constexpr NodeId NODE_NIL = static_cast<NodeId>(-1);
    struct Node {
        T value{};
        NodeId left = NODE_NIL;
        NodeId right = NODE_NIL;

        Node() = default;

        explicit Node(T value)
            : value{std::move(value)}
        {
        }

        Node(T value, NodeId left, NodeId right)
            : value{std::move(value)},
              left{left},
              right{right}
        {
        }
    };

    std::vector<Node> m_nodes;
    T m_identity;
    std::size_t m_size;
    NodeId m_init_id;
    Operator m_op;

    [[nodiscard]] constexpr Node& get_node(NodeId id)
    {
        return m_nodes.at(id);
    }

    template<typename... Args>
    [[nodiscard]] constexpr std::size_t make_node(Args&&... args)
    {
        NodeId id = m_nodes.size();
        m_nodes.emplace_back(std::forward<Args>(args)...);
        return id;
    }

    T query(NodeId node_id, std::size_t tl, std::size_t tr, std::size_t l,
            std::size_t r)
    {
        if (l > r)
            return m_identity;

        Node& node = get_node(node_id);

        if (tl == l && tr == r)
            return node.value;

        std::size_t tm = (tl + tr) / 2;
        return m_op(query(node.left, tl, tm, l, std::min(r, tm)),
                    query(node.right, tm + 1, tr, std::max(l, tm + 1), r));
    }

    [[nodiscard]] NodeId update(NodeId node_id, std::size_t tl, std::size_t tr,
                                std::size_t pos, T value)
    {
        Node& node = get_node(node_id);

        if (tl == tr) {
            assert(tl == pos);
            assert(node.left == NODE_NIL);
            assert(node.right == NODE_NIL);
            return make_node(std::move(value));
        }

        NodeId new_left = node.left;
        NodeId new_right = node.right;

        std::size_t tm = (tl + tr) / 2;
        if (pos <= tm)
            new_left = update(new_left, tl, tm, pos, std::move(value));
        else
            new_right = update(new_right, tm + 1, tr, pos, std::move(value));

        return make_node(
            m_op(get_node(new_left).value, get_node(new_right).value), new_left,
            new_right);
    }

    [[nodiscard]] constexpr NodeId get_version_root(Version v)
    {
        return v.m_root_id;
    }

    [[nodiscard]] NodeId node_from_data(std::initializer_list<T> data,
                                        std::size_t tl, std::size_t tr)
    {
        if (tl > tr)
            throw std::invalid_argument("tl cannot be greater than tr");

        if (tl == tr)
            return make_node(*(data.begin() + tl));

        std::size_t tm = (tl + tr) / 2;
        NodeId left = node_from_data(data, tl, tm);
        NodeId right = node_from_data(data, tm + 1, tr);

        return make_node(m_op(get_node(left).value, get_node(right).value),
                         left, right);
    }

    template<typename Iter>
    [[nodiscard]] NodeId node_from_iter(Iter& it, std::size_t tl,
                                        std::size_t tr)
    {
        if (tl > tr)
            throw std::invalid_argument("tl cannot be greater than tr");

        if (tl == tr)
            return make_node(*it++);

        std::size_t tm = (tl + tr) / 2;
        NodeId left = node_from_iter(it, tl, tm);
        NodeId right = node_from_iter(it, tm + 1, tr);

        return make_node(m_op(get_node(left).value, get_node(right).value),
                         left, right);
    }

    [[nodiscard]] NodeId node_from_range(std::size_t tl, std::size_t tr)
    {
        if (tl > tr)
            throw std::invalid_argument("tl cannot be greater than tr");

        if (tl == tr)
            return make_node();

        std::size_t tm = (tl + tr) / 2;
        NodeId left = node_from_range(tl, tm);
        NodeId right = node_from_range(tm + 1, tr);

        return make_node(m_op(get_node(left).value, get_node(right).value),
                         left, right);
    }

    template<typename... Args>
    [[nodiscard]] Version emplace_version(Args&&... args)
    {
        NodeId id = make_node(std::forward<Args>(args)...);
        return Version{id};
    }

public:
    [[nodiscard]] constexpr Version init()
    {
        return Version{m_init_id};
    }

    explicit PersistentSegTree(T identity, std::size_t size,
                               Operator op = Operator{})
        : m_identity{std::move(identity)},
          m_size{size},
          m_op{std::move(op)}
    {
        if (size == 0)
            throw std::invalid_argument("data cannot be empty");

        m_init_id = node_from_range(0, size - 1);
    }

    PersistentSegTree(T identity, std::initializer_list<T> data,
                      Operator op = Operator{})
        : m_identity{std::move(identity)},
          m_size{data.size()},
          m_op{std::move(op)}
    {
        if (m_size == 0)
            throw std::invalid_argument("data cannot be empty");

        m_init_id = node_from_data(data, 0, data.size() - 1);
    }

    template<typename Iter>
    PersistentSegTree(T identity, Iter begin, Iter end,
                      Operator op = Operator{})
        : m_identity{std::move(identity)},
          m_size{static_cast<std::size_t>(std::distance(begin, end))},
          m_op{std::move(op)}
    {
        if (m_size == 0)
            throw std::invalid_argument("data cannot be empty");

        m_init_id = node_from_iter(begin, 0, m_size - 1);
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return m_size;
    }

    T query(Version v, std::size_t l, std::size_t r)
    {
        NodeId root = get_version_root(v);
        return query(root, 0, m_size - 1, l, r);
    }

    [[nodiscard]] Version update(Version v, std::size_t pos, T value)
    {
        NodeId root = get_version_root(v);
        NodeId new_root = update(root, 0, m_size - 1, pos, std::move(value));
        return Version{new_root};
    }
};

#endif
