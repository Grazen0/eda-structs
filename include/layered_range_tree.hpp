#ifndef EDA_STRUCTS_LAYERED_RANGE_TREE_HPP
#define EDA_STRUCTS_LAYERED_RANGE_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

template<typename T>
class LayeredRangeTree {
private:
    using NodeId = std::size_t;
    static constexpr NodeId NIL = static_cast<NodeId>(-1);

    enum class Dir : std::uint8_t {
        LEFT,
        RIGHT
    };

    struct Entry {
        std::ptrdiff_t m_prev_down = -1;
        std::ptrdiff_t m_next_down = -1;
    };

    struct Node {
        T m_left_max;
        T m_right_min;
        NodeId m_left = NIL;
        NodeId m_right = NIL;
        std::vector<T> m_ys;
        std::vector<Entry> m_entries_left;
        std::vector<Entry> m_entries_right;

        explicit Node(T left_max, T right_min)
            : m_left_max{std::move(left_max)},
              m_right_min(std::move(right_min))
        {
        }

        Node(T left_max, T right_min, NodeId left, NodeId right)
            : m_left_max{std::move(left_max)},
              m_right_min(std::move(right_min)),
              m_left{left},
              m_right{right}
        {
        }

        [[nodiscard]] constexpr NodeId& child(Dir dir)
        {
            return dir == Dir::LEFT ? m_left : m_right;
        }

        [[nodiscard]] constexpr const NodeId& child(Dir dir) const
        {
            return dir == Dir::LEFT ? m_left : m_right;
        }

        [[nodiscard]] constexpr std::vector<Entry>& entries(Dir dir)
        {
            return dir == Dir::LEFT ? m_entries_left : m_entries_right;
        }

        [[nodiscard]] constexpr const std::vector<Entry>& entries(Dir dir) const
        {
            return dir == Dir::LEFT ? m_entries_left : m_entries_right;
        }
    };

    std::vector<Node> m_nodes;
    NodeId m_root = NIL;
    std::size_t m_size;
    T m_min_x;
    T m_max_x;

    template<typename... Args>
    [[nodiscard]] constexpr NodeId make_node(Args&&... args)
    {
        m_nodes.emplace_back(std::forward<Args>(args)...);
        return m_nodes.size() - 1;
    }

    [[nodiscard]] Node& get_node(NodeId id)
    {
        return m_nodes.at(id);
    }

    [[nodiscard]] const Node& get_node(NodeId id) const
    {
        return m_nodes.at(id);
    }

    template<typename Iter>
    [[nodiscard]]
    std::tuple<NodeId, T, T> node_from_iter(Iter& it, std::size_t l,
                                            std::size_t r)
    {
        assert(l <= r);

        if (l == r) {
            auto val = *it++;
            // NOTE: could make these be N/A or something
            NodeId nid = make_node(val.first, val.first);
            get_node(nid).m_ys.emplace_back(val.second);
            return std::make_tuple(nid, val.first, val.first);
        }

        std::size_t mid = (l + r) / 2;

        auto [lid, left_min, left_max] = node_from_iter<Iter>(it, l, mid);
        auto [rid, right_min, right_max] = node_from_iter<Iter>(it, mid + 1, r);

        NodeId nid = make_node(left_max, right_min, lid, rid);
        Node& node = get_node(nid);
        Node& left = get_node(lid);
        Node& right = get_node(rid);

        auto& a = left.m_ys;
        auto& b = right.m_ys;
        auto& out = node.m_ys;
        node.m_ys.reserve(a.size() + b.size());

        std::vector<std::pair<Dir, std::size_t>> down_ptrs;
        down_ptrs.reserve(a.size() + b.size());

        std::size_t i = 0;
        std::size_t j = 0;

        while (i < a.size() || j < b.size()) {
            if (j >= b.size() || (i < a.size() && a.at(i) < b.at(j))) {
                down_ptrs.emplace_back(Dir::LEFT, i);
                out.emplace_back(a.at(i++));
            } else {
                down_ptrs.emplace_back(Dir::RIGHT, j);
                out.emplace_back(b.at(j++));
            }
        }

        node.m_entries_left.resize(a.size() + b.size());
        node.m_entries_right.resize(a.size() + b.size());

        std::ptrdiff_t last_l = -1;
        std::ptrdiff_t last_r = -1;

        for (std::size_t i = 0; i < node.m_entries_left.size(); ++i) {
            auto& entry_left = node.m_entries_left.at(i);
            auto& entry_right = node.m_entries_right.at(i);

            if (down_ptrs.at(i).first == Dir::LEFT)
                last_l = down_ptrs.at(i).second;
            else
                last_r = down_ptrs.at(i).second;

            entry_left.m_prev_down = last_l;
            entry_right.m_prev_down = last_r;
        }

        last_l = left.m_ys.size();
        last_r = right.m_ys.size();

        for (std::size_t i = node.m_ys.size(); i > 0; --i) {
            auto& entry_left = node.m_entries_left.at(i - 1);
            auto& entry_right = node.m_entries_right.at(i - 1);

            if (down_ptrs.at(i - 1).first == Dir::LEFT)
                last_l = down_ptrs.at(i - 1).second;
            else
                last_r = down_ptrs.at(i - 1).second;

            entry_left.m_next_down = last_l;
            entry_right.m_next_down = last_r;
        }

        return std::make_tuple(nid, left_min, right_max);
    }

    struct CascadeRange {
        std::ptrdiff_t l{};
        std::ptrdiff_t r{};

        template<typename Iter>
        CascadeRange(Iter begin, Iter end, T lb, T rb)
            : l{std::lower_bound(begin, end, lb) - begin},
              r{std::upper_bound(begin, end, rb) - begin - 1}
        {
            assert(size() >= 0);
        }

        [[nodiscard]] std::size_t size() const
        {
            return r - l + 1;
        }
    };

    [[nodiscard]] CascadeRange cascade(const Node& from, Dir dir,
                                       CascadeRange range) const
    {
        const auto& entries = from.entries(dir);
        const Node& child = get_node(from.child(dir));

        if (range.l < static_cast<std::ptrdiff_t>(entries.size()))
            range.l = entries.at(range.l).m_next_down;
        else
            range.l = child.m_ys.size();

        if (range.r >= 0)
            range.r = entries.at(range.r).m_prev_down;
        else
            range.r = -1;

        return range;
    }

public:
    LayeredRangeTree(std::initializer_list<std::pair<T, T>> data)
        : LayeredRangeTree{data.begin(), data.end()}
    {
    }

    template<typename Iter>
    LayeredRangeTree(Iter begin, Iter end)
        : m_size{static_cast<std::size_t>(std::distance(begin, end))},
          m_min_x{begin->first},
          m_max_x{(end - 1)->first}
    {
        if (!std::is_sorted(begin, end))
            throw std::invalid_argument("data must be sorted by x");

        std::tie(m_root, m_min_x, m_max_x) =
            node_from_iter(begin, 0, m_size - 1);
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return m_size;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_size == 0;
    }

    [[nodiscard]] std::size_t query(T l1, T r1, T l2, T r2) const
    {
        if (l1 > r1 || l2 > r2)
            return 0;

        const Node& root = get_node(m_root);

        CascadeRange lrange{root.m_ys.begin(), root.m_ys.end(), l2, r2};
        CascadeRange rrange{lrange};

        if (l1 <= m_min_x && r1 >= m_max_x)
            return lrange.size();

        std::size_t count = 0;

        NodeId lnid = m_root;
        NodeId rnid = m_root;

        if (l1 > m_min_x && r1 < m_max_x) {
            while (lnid == rnid) {
                const Node& lnode = get_node(lnid);
                const Node& rnode = get_node(rnid);

                Dir ldir = lnode.m_right_min >= l1 ? Dir::LEFT : Dir::RIGHT;
                lnid = lnode.child(ldir);
                lrange = cascade(lnode, ldir, lrange);

                Dir rdir = rnode.m_left_max <= r1 ? Dir::RIGHT : Dir::LEFT;
                rnid = rnode.child(rdir);
                rrange = cascade(rnode, rdir, rrange);

                assert(lrange.size() >= 0);
                assert(rrange.size() >= 0);
            }
        }

        if (l1 > m_min_x) {
            while (true) {
                const Node& lnode = get_node(lnid);
                if (lnode.m_right == NIL)
                    break;

                Dir dir = lnode.m_right_min >= l1 ? Dir::LEFT : Dir::RIGHT;
                if (dir == Dir::LEFT) {
                    CascadeRange lrrange = cascade(lnode, Dir::RIGHT, lrange);
                    count += lrrange.size();
                }

                lnid = lnode.child(dir);
                lrange = cascade(lnode, dir, lrange);
            }
        }

        if (r1 < m_max_x) {
            while (true) {
                const Node& rnode = get_node(rnid);
                if (rnode.m_left == NIL)
                    break;

                Dir dir = rnode.m_left_max <= r1 ? Dir::RIGHT : Dir::LEFT;
                if (dir == Dir::RIGHT) {
                    CascadeRange rlrange = cascade(rnode, Dir::LEFT, rrange);
                    count += rlrange.size();
                }

                rnid = rnode.child(dir);
                rrange = cascade(rnode, dir, rrange);
            }
        }

        return count;
    }
};

#endif
