#ifndef EDA_STRUCTS_FIBONACCI_HEAP_HPP
#define EDA_STRUCTS_FIBONACCI_HEAP_HPP

#include <cassert>
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

    Compare cmp{};
    Node* max = nullptr;
    std::size_t count = 0;

    void merge_with_max(Node* max_other)
    {
        if (max_other == nullptr)
            return;

        node_assert_valid(max_other);

        if (max == nullptr) {
            max = max_other;
            return;
        }

        list_merge(max, max_other);

        if (cmp(max->value, max_other->value))
            max = max_other;
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

        if (cmp(a->value, b->value))
            std::swap(a, b);

        // a >= b
        ++a->rank;
        b->parent = a;

        if (a->child == nullptr) {
            a->child = b;
            return a;
        }

        list_merge(a->child, b);
        return a;
    }

    void consolidate()
    {
        assert(max != nullptr);

        Node* begin = std::exchange(max, nullptr);
        Node* cur = begin;
        std::vector<Node*> merged;

        do {
            assert(cur->parent == nullptr);

            Node* next = cur->next;
            cur->prev = cur;
            cur->next = cur;

            Node* m = cur;

            while (m->rank < merged.size() && merged[m->rank] != nullptr) {
                Node* to_merge = std::exchange(merged[m->rank], nullptr);
                m = link_nodes(to_merge, m);
            }

            if (m->rank >= merged.size())
                merged.resize(m->rank + 1);

            merged[m->rank] = m;

            cur = next;
        } while (cur != begin);

        for (auto* root : merged)
            merge_with_max(root);
    }

    void cut(Node* x)
    {
        node_assert_valid(x);

        Node* p = x->parent;
        assert(p->child != nullptr);
        assert(p->rank > 0);

        p->child = list_remove(x);
        --p->rank;

        x->parent = nullptr;
        list_merge(max, x);
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
        list_destroy(max);
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
        if (max == nullptr)
            return std::nullopt;

        return max->value;
    }

    void merge(FibonacciHeap other)
    {
        merge_with_max(std::exchange(other.max, nullptr));
    }

    constexpr iterator insert(T value)
    {
        return emplace(std::move(value));
    }

    template<typename... Args>
    iterator emplace(Args&&... args)
    {
        auto* node = new Node{std::forward<Args>(args)...};
        merge_with_max(node);
        ++count;

        return iterator{node};
    }

    std::optional<T> pop()
    {
        if (max == nullptr)
            return std::nullopt;

        // Merge children into roots
        if (Node* child = std::exchange(max->child, nullptr)) {
            Node* cur = child;
            do {
                cur->parent = nullptr;
                cur = cur->next;
            } while (cur != child);

            list_merge(max, child);
        }

        T retval = std::move(max->value);
        delete std::exchange(max, list_remove(max));

        if (max != nullptr)
            consolidate();

        --count;
        return retval;
    }

    bool increase_key(const iterator& it, T new_value)
    {
        if (cmp(new_value, *it) || *it == new_value)
            return false;

        Node* x = it.node;
        x->value = std::move(new_value);

        Node* p = x->parent;

        if (p != nullptr && cmp(p->value, x->value)) {
            cut(x);
            cascading_cut(p);
        }

        if (cmp(max->value, x->value))
            max = x;

        return true;
    }

    constexpr void swap(FibonacciHeap& other) noexcept
    {
        std::swap(max, other.max);
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return count;
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return count == 0;
    }
};

#endif
