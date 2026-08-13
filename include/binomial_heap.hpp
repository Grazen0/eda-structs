#ifndef EDA_STRUCTS_BINOMIAL_HEAP_HPP
#define EDA_STRUCTS_BINOMIAL_HEAP_HPP

#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <stdexcept>
#include <utility>

template<typename T, typename Compare = std::less<T>>
class BinomialHeap {
    struct Node {
        T value;
        std::size_t rank;
        Node* sibling = nullptr;
        Node* child = nullptr;

        explicit Node(T value, std::size_t rank)
            : value{std::move(value)},
              rank{rank}
        {
        }
    };

    std::list<Node*> roots;
    Compare cmp{};

    static void node_delete(Node* node)
    {
        if (node == nullptr)
            return;

        node_delete(node->sibling);
        node_delete(node->child);
        delete node;
    }

    [[nodiscard]] Node* merge_roots(Node* a, Node* b)
    {
        assert(a != nullptr);
        assert(b != nullptr);
        assert(a->rank == b->rank);
        assert(a->sibling == nullptr);
        assert(b->sibling == nullptr);

        if (cmp(b->value, a->value))
            return merge_roots(b, a);

        // we now have root(a) <= root(b)

        a->sibling = b->child;
        b->child = a;
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
                *it = merge_roots(*it, *next);
                roots.erase(next);
            }
        }
    }

public:
    BinomialHeap() = default;

    BinomialHeap(BinomialHeap& other) = delete;

    BinomialHeap(BinomialHeap&& other) noexcept
    {
        swap(other);
    }

    BinomialHeap& operator=(BinomialHeap& other) = delete;

    BinomialHeap& operator=(BinomialHeap&& other) noexcept
    {
        swap(other);
        return *this;
    }

    ~BinomialHeap()
    {
        for (auto root : roots)
            node_delete(root);
    }

    void insert(T value)
    {
        roots.emplace_front(new Node{std::move(value), 1});
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

        Node* max_root = *max_it;
        T out = std::move(max_root->value);
        roots.erase(max_it);

        auto it = roots.rbegin();
        Node* cur_child = max_root->child;
        delete std::exchange(max_root, nullptr);

        while (it != roots.rend() && cur_child != nullptr) {
            if (cur_child->rank > (*it)->rank) {
                Node* next = std::exchange(cur_child->sibling, nullptr);
                roots.insert(it.base(), cur_child);
                cur_child = next;
            } else {
                ++it;
            }
        }

        while (cur_child != nullptr) {
            Node* next = std::exchange(cur_child->sibling, nullptr);
            roots.push_front(cur_child);
            cur_child = next;
        }

        collapse_roots();
        return out;
    }

    void merge(BinomialHeap other)
    {
        std::list<Node*> roots_a = std::move(roots);
        std::list<Node*> roots_b = std::move(other.roots);
        roots.clear();

        auto it_a = roots_a.begin();
        auto it_b = roots_b.begin();

        while (it_a != roots_a.end() && it_b != roots_b.end()) {
            if ((*it_a)->rank < (*it_b)->rank)
                roots.emplace_back(*it_a++);
            else
                roots.emplace_back(*it_b++);
        }

        while (it_a != roots_a.end())
            roots.emplace_back(*it_a++);

        while (it_b != roots_b.end())
            roots.emplace_back(*it_b++);

        collapse_roots();
    }

    void swap(BinomialHeap& other) noexcept
    {
        std::swap(roots, other.roots);
    }
};

#endif
