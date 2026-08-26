#ifndef EDA_STRUCTS_PERSISTENT_SEGMENT_TREE_HPP
#define EDA_STRUCTS_PERSISTENT_SEGMENT_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

template<typename T, typename Operator, T IDENTITY>
class PersistentSegmentTree {
public:
    class Version {
    private:
        explicit Version(std::size_t idx)
            : idx{idx}
        {
        }

        std::size_t idx;

        friend class PersistentSegmentTree;
    };

private:
    using NodeId = std::size_t;
    struct Node {
        T value;
        std::optional<NodeId> left = std::nullopt;
        std::optional<NodeId> right = std::nullopt;
    };

    std::vector<Node> nodes_;
    std::vector<NodeId> roots_;
    std::size_t size_;
    Operator op_;

    [[nodiscard]] constexpr Node& get_node(NodeId id)
    {
        return nodes_.at(id);
    }

    template<typename... Args>
    [[nodiscard]] constexpr std::size_t make_node(Args&&... args)
    {
        NodeId id = nodes_.size();
        nodes_.emplace_back(std::forward<Args>(args)...);
        return id;
    }

    T query(NodeId node_id, std::size_t tl, std::size_t tr, std::size_t l,
            std::size_t r)
    {
        if (l > r)
            return IDENTITY;

        Node& node = get_node(node_id);

        if (tl == l && tr == r)
            return node.value;

        std::size_t tm = (tl + tr) / 2;
        return op_(query(*node.left, tl, tm, l, std::min(r, tm)),
                   query(*node.right, tm + 1, tr, std::max(l, tm + 1), r));
    }

    [[nodiscard]] NodeId update(NodeId node_id, std::size_t tl, std::size_t tr,
                                std::size_t pos, T value)
    {
        Node& node = get_node(node_id);

        if (tl == tr) {
            assert(tl == pos);
            assert(!node.left);
            assert(!node.right);
            return make_node(std::move(value));
        }

        NodeId new_left = *node.left;
        NodeId new_right = *node.right;

        std::size_t tm = (tl + tr) / 2;
        if (pos <= tm)
            new_left = update(new_left, tl, tm, pos, std::move(value));
        else
            new_right = update(new_right, tm + 1, tr, pos, std::move(value));

        return make_node(
            op_(get_node(new_left).value, get_node(new_right).value), new_left,
            new_right);
    }

    [[nodiscard]] constexpr std::size_t get_version_root(Version v)
    {
        if (v.idx >= roots_.size())
            throw std::invalid_argument("invalid version");

        return roots_.at(v.idx);
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

        return make_node(op_(get_node(left).value, get_node(right).value), left,
                         right);
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

        return make_node(op_(get_node(left).value, get_node(right).value), left,
                         right);
    }

    [[nodiscard]] Version make_version(std::size_t new_root)
    {
        std::size_t v = roots_.size();
        roots_.emplace_back(new_root);
        return Version{v};
    }

public:
    [[nodiscard]] static constexpr Version init()
    {
        return Version{0};
    }

    explicit PersistentSegmentTree(std::size_t size, Operator op = Operator{})
        : size_{size},
          op_{std::move(op)}
    {
        if (size == 0)
            throw std::invalid_argument("data cannot be empty");

        roots_.push_back(node_from_range(0, size - 1));
    }

    PersistentSegmentTree(std::initializer_list<T> data,
                          Operator op = Operator{})
        : size_{data.size()},
          op_{std::move(op)}
    {
        if (data.size() == 0)
            throw std::invalid_argument("data cannot be empty");

        roots_.push_back(node_from_data(data, 0, data.size() - 1));
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return size_;
    }

    T query(Version v, std::size_t l, std::size_t r)
    {
        NodeId root = get_version_root(v);
        return query(root, 0, size_ - 1, l, r);
    }

    [[nodiscard]] Version update(Version v, std::size_t pos, T value)
    {
        NodeId root = get_version_root(v);
        NodeId new_root = update(root, 0, size_ - 1, pos, std::move(value));
        return make_version(new_root);
    }
};

#endif
