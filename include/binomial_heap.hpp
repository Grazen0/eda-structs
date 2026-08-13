#ifndef EDA_STRUCTS_BINOMIAL_HEAP_HPP
#define EDA_STRUCTS_BINOMIAL_HEAP_HPP

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <stdexcept>
#include <utility>

template<typename T, typename Compare = std::less<T>>
class BinomialHeap {
    struct Node {
        T value;
        std::size_t rank;
        std::unique_ptr<Node> sibling;
        std::unique_ptr<Node> child;

        explicit Node(T value, std::size_t rank)
            : value{std::move(value)},
              rank{rank}
        {
        }
    };

    std::list<std::unique_ptr<Node>> roots;
    Compare cmp{};

    [[nodiscard]] std::unique_ptr<Node> merge_roots(std::unique_ptr<Node> a,
                                                    std::unique_ptr<Node> b)
    {
        assert(a->rank == b->rank);
        assert(a->sibling == nullptr);
        assert(b->sibling == nullptr);

        if (cmp(b->value, a->value))
            std::swap(a, b);

        // we now have root(a) <= root(b)

        auto b_child_prev = std::exchange(b->child, std::move(a));
        b->child->sibling = std::move(b_child_prev);
        ++b->rank;

        return b;
    }

    decltype(roots)::const_iterator find_max() const
    {
        if (roots.empty())
            throw std::out_of_range{"binomial heap is empty"};

        auto out = roots.begin();

        for (auto it = roots.begin(); it != roots.end(); ++it) {
            if (cmp((*out)->value, (*it)->value))
                out = it;
        }

        return out;
    }

    void collapse_roots()
    {
        auto it = roots.begin();

        while (it != roots.end()) {
            auto next = std::next(it);
            if (next == roots.end())
                break;

            if ((*it)->rank != (*next)->rank) {
                ++it;
            } else {
                *it = merge_roots(std::move(*it), std::move(*next));
                roots.erase(next);
            }
        }
    }

public:
    void insert(T value)
    {
        roots.emplace_front(std::make_unique<Node>(std::move(value), 1));
        collapse_roots();
    }

    [[nodiscard]] const T& peek() const
    {
        auto max_it = find_max();
        return (*max_it)->value;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return roots.empty();
    }

    [[nodiscard]] T pop()
    {
        auto max_it = find_max();

        auto it = roots.rbegin();
        auto cur_child = std::move((*max_it)->child);
        T out = std::move((*max_it)->value);

        roots.erase(max_it);

        while (it != roots.rend() && cur_child != nullptr) {
            if (cur_child->rank > (*it)->rank) {
                auto next = std::move(cur_child->sibling);
                roots.insert(it.base(), std::move(cur_child));
                cur_child = std::move(next);
            } else {
                ++it;
            }
        }

        while (cur_child != nullptr) {
            auto next = std::move(cur_child->sibling);
            roots.push_front(std::move(cur_child));
            cur_child = std::move(next);
        }

        collapse_roots();
        return out;
    }

    void merge(BinomialHeap other)
    {
        decltype(roots) roots_a = std::move(roots);
        decltype(roots) roots_b = std::move(other.roots);
        roots.clear();

        auto it_a = roots_a.begin();
        auto it_b = roots_b.begin();

        while (it_a != roots_a.end() && it_b != roots_b.end()) {
            if ((*it_a)->rank < (*it_b)->rank)
                roots.emplace_back(std::move(*it_a++));
            else
                roots.emplace_back(std::move(*it_b++));
        }

        while (it_a != roots_a.end())
            roots.emplace_back(std::move(*it_a++));

        while (it_b != roots_b.end())
            roots.emplace_back(std::move(*it_b++));

        collapse_roots();
    }

    void swap(BinomialHeap& other) noexcept
    {
        std::swap(roots, other.roots);
    }
};

#endif
