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
        Node* child = nullptr;
        Node* prev = this;
        Node* next = this;

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
        assert(node->child == nullptr);

        if (node->next == node) {
            delete node;
            return nullptr;
        }
        Node* next = node->next;

        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;

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

    [[nodiscard]] Node* link(Node* a, Node* b)
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
            Node* next = cur->next;
            cur->prev = cur;
            cur->next = cur;

            Node* m = cur;

            while (m->rank < merged.size() && merged[m->rank] != nullptr) {
                Node* to_merge = std::exchange(merged[m->rank], nullptr);
                m = link(to_merge, m);
            }

            if (m->rank >= merged.size())
                merged.resize(m->rank + 1);

            merged[m->rank] = m;

            cur = next;
        } while (cur != begin);

        for (auto* root : merged)
            merge_with_max(root);
    }

public:
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

    constexpr void insert(T value)
    {
        emplace(std::move(value));
    }

    template<typename... Args>
    void emplace(Args&&... args)
    {
        auto* node = new Node{std::forward<Args>(args)...};
        merge_with_max(node);
        ++count;
    }

    std::optional<T> pop()
    {
        if (max == nullptr)
            return std::nullopt;

        // Merge children into roots
        if (Node* child = std::exchange(max->child, nullptr))
            list_merge(max, child);

        T retval = std::move(max->value);
        max = list_remove(max);

        if (max != nullptr)
            consolidate();

        --count;
        return retval;
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
