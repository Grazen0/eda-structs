#ifndef EDA_STRUCTS_FIBONACCI_HEAP_HPP
#define EDA_STRUCTS_FIBONACCI_HEAP_HPP

#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

template<typename T, typename Compare = std::less<T>>
class FibonacciHeap {
private:
    struct Node {
        T value;
        std::size_t rank = 0;
        Node* parent = nullptr;
        Node* child = nullptr;
        Node* prev = this;
        Node* next = this;
        bool marked = false;

        template<typename... Args>
        explicit Node(Args&&... args)
            : value{std::forward<Args>(args)...}
        {
        }
    };

    static constexpr void node_assert_valid(const Node* node)
    {
        assert(node != nullptr);
        assert(node->prev != nullptr);
        assert(node->next != nullptr);
    }

    static Node* list_remove(Node* node)
    {
        node_assert_valid(node);

        if (node->next == node)
            return nullptr;

        Node* next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;

        node->next = node;
        node->prev = node;

        return next;
    }

    static void list_merge(Node* a, Node* b)
    {
        node_assert_valid(a);
        node_assert_valid(b);

        Node* tail_a = a->prev;
        Node* tail_b = b->prev;

        a->prev = tail_b;
        tail_b->next = a;

        b->prev = tail_a;
        tail_a->next = b;
    }

    static void list_destroy(Node* node)
    {
        if (node == nullptr)
            return;

        Node* begin = node;
        Node* cur = begin;

        do {
            Node* next = cur->next;
            list_destroy(std::exchange(cur->child, nullptr));
            delete std::exchange(cur, cur->next);
            cur = next;
        } while (cur != begin);
    }

    Compare m_cmp{};
    Node* m_min = nullptr;
    std::size_t m_count = 0;

    void merge_with_min(Node* min_other)
    {
        if (min_other == nullptr)
            return;

        node_assert_valid(min_other);

        if (m_min == nullptr) {
            m_min = min_other;
            return;
        }

        list_merge(m_min, min_other);

        if (m_cmp(min_other->value, m_min->value))
            m_min = min_other;
    }

    [[nodiscard]] Node* link_nodes(Node* a, Node* b)
    {
        if (a != nullptr)
            node_assert_valid(a);

        if (b != nullptr)
            node_assert_valid(b);

        if (a == nullptr)
            return b;

        if (b == nullptr)
            return a;

        if (m_cmp(b->value, a->value))
            std::swap(a, b);

        // a <= b
        ++a->rank;
        b->parent = a;

        if (a->child == nullptr)
            a->child = b;
        else
            list_merge(a->child, b);

        return a;
    }

    void consolidate()
    {
        assert(m_min != nullptr);

        Node* begin = std::exchange(m_min, nullptr);
        Node* cur = begin;
        std::vector<Node*> merged(2 * std::log2(m_count));

        do {
            assert(cur->parent == nullptr);
            assert(!cur->marked);

            Node* next = cur->next;
            cur->prev = cur;
            cur->next = cur;

            Node* m = cur;

            while (merged[m->rank] != nullptr) {
                Node* to_merge = std::exchange(merged[m->rank], nullptr);
                m = link_nodes(to_merge, m);
            }

            merged[m->rank] = m;

            cur = next;
        } while (cur != begin);

        for (auto* root : merged)
            merge_with_min(root);
    }

    void cut(Node* x)
    {
        node_assert_valid(x);

        Node* p = x->parent;
        assert(p != nullptr);
        assert(p->child != nullptr);
        assert(p->rank > 0);

        p->child = list_remove(x);
        --p->rank;

        x->parent = nullptr;
        list_merge(m_min, x);
        x->marked = false;
    }

    void cascading_cut(Node* y)
    {
        node_assert_valid(y);
        Node* z = y->parent;

        if (z != nullptr) {
            if (!y->marked) {
                y->marked = true;
            } else {
                cut(y);
                cascading_cut(z);
            }
        }
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

        friend class FibonacciHeap;
    };

    FibonacciHeap() = default;

    FibonacciHeap(FibonacciHeap& other) = delete;

    FibonacciHeap(FibonacciHeap&& other) noexcept
    {
        swap(other);
    }

    ~FibonacciHeap()
    {
        list_destroy(m_min);
    }

    FibonacciHeap& operator=(FibonacciHeap& other) = delete;

    FibonacciHeap& operator=(FibonacciHeap&& other) noexcept
    {
        swap(other);
        return *this;
    }

    [[nodiscard]] constexpr std::optional<std::reference_wrapper<const T>>
    peek() const
    {
        if (m_min == nullptr)
            return std::nullopt;

        return m_min->value;
    }

    void merge(FibonacciHeap other)
    {
        merge_with_min(std::exchange(other.m_min, nullptr));
        m_count += other.m_count;
    }

    constexpr iterator insert(T value)
    {
        return emplace(std::move(value));
    }

    template<typename... Args>
    iterator emplace(Args&&... args)
    {
        auto* node = new Node{std::forward<Args>(args)...};
        merge_with_min(node);
        ++m_count;

        return iterator{node};
    }

    std::optional<T> pop()
    {
        if (m_min == nullptr)
            return std::nullopt;

        // Merge children into roots
        if (Node* child = std::exchange(m_min->child, nullptr)) {
            Node* cur = child;
            do {
                cur->parent = nullptr;
                cur->marked = false;
                cur = cur->next;
            } while (cur != child);

            list_merge(m_min, child);
        }

        T retval = std::move(m_min->value);
        delete std::exchange(m_min, list_remove(m_min));

        if (m_min != nullptr)
            consolidate();

        --m_count;
        return retval;
    }

    bool decrease_key(const iterator& it, T new_value)
    {
        if (!m_cmp(new_value, *it))
            return false;

        Node* x = it.node;
        x->value = std::move(new_value);

        Node* p = x->parent;

        if (p != nullptr && m_cmp(x->value, p->value)) {
            cut(x);
            cascading_cut(p);
        }

        if (m_cmp(x->value, m_min->value))
            m_min = x;

        return true;
    }

    constexpr void swap(FibonacciHeap& other) noexcept
    {
        std::swap(m_min, other.m_min);
        std::swap(m_count, other.m_count);
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return m_count;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_count == 0;
    }
};

#endif
