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
        std::size_t rank = 1;
        Node* sibling = nullptr;
        Node* child = nullptr;

        explicit Node(T value)
            : value{std::move(value)}
        {
        }
    };

    static void node_destroy(Node* node)
    {
        if (node == nullptr)
            return;

        node_destroy(node->sibling);
        node_destroy(node->child);
        delete node;
    }

    std::list<Node*> m_roots;
    Compare m_cmp{};

    void binomial_link(Node* y, Node* z)
    {
        y->sibling = z->child;
        z->child = y;
        ++z->rank;
    }

    [[nodiscard]] Node* merge_roots(Node* a, Node* b)
    {
        assert(a->rank == b->rank);
        assert(a->sibling == nullptr);
        assert(b->sibling == nullptr);

        if (m_cmp(a->value, b->value))
            std::swap(a, b);

        // we now have root(a) <= root(b)

        a->sibling = b->child;
        b->child = a;
        ++b->rank;

        return b;
    }

    [[nodiscard]] typename decltype(m_roots)::const_iterator find_max() const
    {
        if (m_roots.empty())
            throw std::out_of_range{"binomial heap is empty"};

        auto out = m_roots.begin();

        for (auto it = m_roots.begin(); it != m_roots.end(); ++it) {
            if (m_cmp((*it)->value, (*out)->value))
                out = it;
        }

        return out;
    }

public:
    class iterator {
    private:
        Node* node;

        explicit iterator(Node* node)
            : node{node}
        {
        }

    public:
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;

        bool operator==(const iterator& other) const
        {
            return node == other.node;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

        reference operator*() const
        {
            return node->value;
        }

        friend class BinomialHeap;
    };

    BinomialHeap() = default;

    BinomialHeap(BinomialHeap&& other) noexcept
    {
        swap(other);
    }

    BinomialHeap(BinomialHeap& other) = delete;

    BinomialHeap& operator=(BinomialHeap&& other) noexcept
    {
        swap(other);
        return *this;
    }

    BinomialHeap& operator=(BinomialHeap& other) = delete;

    ~BinomialHeap()
    {
        for (Node* root : m_roots)
            node_destroy(root);
    }

    iterator insert(T value)
    {
        auto* node = new Node{std::move(value)};
        m_roots.emplace_front(node);

        auto it = m_roots.begin();

        while (it != m_roots.end()) {
            auto next = std::next(it);
            if (next == m_roots.end())
                break;

            if ((*it)->rank != (*next)->rank) {
                assert((*it)->rank < (*next)->rank);
                break;
            }

            *it = merge_roots(*it, *next);
            m_roots.erase(next);
        }

        return iterator{node};
    }

    [[nodiscard]] const T& peek() const
    {
        auto max_it = find_max();
        return (*max_it)->value;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_roots.empty();
    }

    [[nodiscard]] T pop()
    {
        auto max_it = find_max();
        T out = std::move((*max_it)->value);

        BinomialHeap tmp{};
        Node* cur = std::exchange((*max_it)->child, nullptr);
        m_roots.erase(max_it);

        while (cur != nullptr) {
            Node* next = std::exchange(cur->sibling, nullptr);
            tmp.m_roots.emplace_front(cur);
            cur = next;
        }

        merge(std::move(tmp));
        return out;
    }

    void merge(BinomialHeap other)
    {
        auto roots_a = std::move(m_roots);
        auto roots_b = std::move(other.m_roots);
        m_roots.clear();

        auto insert_merging = [&](Node* node) {
            if (!m_roots.empty() && m_roots.back()->rank == node->rank)
                m_roots.back() = merge_roots(node, m_roots.back());
            else
                m_roots.emplace_back(node);
        };

        auto a = roots_a.begin();
        auto b = roots_b.begin();

        while (a != roots_a.end() && b != roots_b.end()) {
            if ((*a)->rank == (*b)->rank)
                m_roots.push_back(merge_roots(*a++, *b++));
            else if ((*a)->rank < (*b)->rank)
                insert_merging(*a++);
            else
                insert_merging(*b++);
        }

        while (a != roots_a.end())
            insert_merging(*a++);

        while (b != roots_b.end())
            insert_merging(*b++);
    }

    void swap(BinomialHeap& other) noexcept
    {
        std::swap(m_roots, other.m_roots);
    }
};

#endif
