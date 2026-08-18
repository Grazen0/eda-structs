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
        std::size_t rank = 1;
        std::unique_ptr<Node> sibling;
        std::unique_ptr<Node> child;

        explicit Node(T value)
            : value{std::move(value)}
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

    [[nodiscard]] decltype(roots)::const_iterator find_max() const
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

public:
    void insert(T value)
    {
        roots.emplace_front(std::make_unique<Node>(std::move(value)));

        auto it = roots.begin();

        while (it != roots.end()) {
            auto next = std::next(it);
            if (next == roots.end())
                break;

            if ((*it)->rank != (*next)->rank) {
                assert((*it)->rank < (*next)->rank);
                break;
            }

            *it = merge_roots(std::move(*it), std::move(*next));
            roots.erase(next);
        }
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
        T out = std::move((*max_it)->value);

        BinomialHeap tmp{};
        std::unique_ptr<Node> cur = std::move((*max_it)->child);
        roots.erase(max_it);

        while (cur) {
            auto next = std::move(cur->sibling);
            tmp.roots.emplace_front(std::move(cur));
            cur = std::move(next);
        }

        merge(std::move(tmp));
        return out;
    }

    void merge(BinomialHeap other)
    {
        auto roots_a = std::move(roots);
        auto roots_b = std::move(other.roots);
        roots.clear();

        auto insert_merging = [&](std::unique_ptr<Node> node) {
            if (!roots.empty() && roots.back()->rank == node->rank)
                roots.back() =
                    merge_roots(std::move(node), std::move(roots.back()));
            else
                roots.emplace_back(std::move(node));
        };

        auto a = roots_a.begin();
        auto b = roots_b.begin();

        while (a != roots_a.end() && b != roots_b.end()) {
            if ((*a)->rank == (*b)->rank)
                roots.push_back(merge_roots(std::move(*a++), std::move(*b++)));
            else if ((*a)->rank < (*b)->rank)
                insert_merging(std::move(*a++));
            else
                insert_merging(std::move(*b++));
        }

        while (a != roots_a.end())
            insert_merging(std::move(*a++));

        while (b != roots_b.end())
            insert_merging(std::move(*b++));
    }

    void swap(BinomialHeap& other) noexcept
    {
        std::swap(roots, other.roots);
    }
};

#endif
