#include "fibonacci_heap.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace
{
    // Repeatedly peek (to check for emptiness) then pop, collecting pop()'s
    // value.
    template<typename T, typename Compare>
    std::vector<T> drain(FibonacciHeap<T, Compare>& heap)
    {
        std::vector<T> out;
        while (auto v = heap.pop())
            out.push_back(std::move(*v));

        return out;
    }

    std::vector<int> random_vector(std::size_t n, unsigned seed)
    {
        std::mt19937 rng{seed};
        std::uniform_int_distribution<int> dist{-1'000'000, 1'000'000};
        std::vector<int> v(n);
        std::generate(v.begin(), v.end(), [&] { return dist(rng); });
        return v;
    }

} // namespace

// ---------------------------------------------------------------------
// Construction / empty-heap behaviour
// ---------------------------------------------------------------------

TEST_CASE("A default-constructed heap is empty", "[fibonacci_heap][basic]")
{
    FibonacciHeap<int> heap;

    SECTION("peek() is std::nullopt")
    {
        REQUIRE(!heap.peek());
    }

    SECTION("pop() is std::nullopt")
    {
        REQUIRE(!heap.pop());
    }
}

// ---------------------------------------------------------------------
// Single-element behaviour
// ---------------------------------------------------------------------

TEST_CASE("Inserting a single element makes it the max",
          "[fibonacci_heap][basic]")
{
    FibonacciHeap<int> heap;
    heap.insert(42);

    REQUIRE(heap.peek() == 42);

    SECTION("pop() removes it and leaves the heap empty")
    {
        REQUIRE(heap.pop() == 42);
        REQUIRE(!heap.peek());
        REQUIRE(!heap.pop());
    }
}

// ---------------------------------------------------------------------
// Multi-element ordering
// ---------------------------------------------------------------------

TEST_CASE("Heap always reports the maximum via peek()",
          "[fibonacci_heap][basic]")
{
    FibonacciHeap<int> heap;
    heap.insert(5);
    REQUIRE(heap.peek() == 5);
    heap.insert(10);
    REQUIRE(heap.peek() == 10);
    heap.insert(1);
    REQUIRE(heap.peek() == 10);
    heap.insert(20);
    REQUIRE(heap.peek() == 20);
    heap.insert(15);
    REQUIRE(heap.peek() == 20);
}

TEST_CASE("Popping repeatedly yields elements in descending order",
          "[fibonacci_heap][basic]")
{
    std::vector<int> values{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    FibonacciHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end(), std::greater<int>{});

    std::vector<int> actual = drain(heap);
    REQUIRE(actual == expected);
}

TEST_CASE("Heap handles duplicate values correctly", "[fibonacci_heap][basic]")
{
    FibonacciHeap<int> heap;
    for (int i = 0; i < 5; ++i)
        heap.insert(7);
    for (int i = 0; i < 3; ++i)
        heap.insert(3);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{7, 7, 7, 7, 7, 3, 3, 3};
    REQUIRE(actual == expected);
}

// ---------------------------------------------------------------------
// Custom comparator
// ---------------------------------------------------------------------

TEST_CASE("A heap with std::greater<int> behaves as a min-heap",
          "[fibonacci_heap][comparator]")
{
    FibonacciHeap<int, std::greater<int>> heap;
    for (int v : {5, 3, 8, 1, 9, 2})
        heap.insert(v);

    REQUIRE(heap.peek() == 1);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{1, 2, 3, 5, 8, 9};
    REQUIRE(actual == expected);
}

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const
    {
        return a.size() < b.size();
    }
};

TEST_CASE("A heap works with a non-trivial value type and custom comparator",
          "[fibonacci_heap][comparator]")
{
    FibonacciHeap<std::string, ByLength> heap;
    heap.insert("a");
    heap.insert("abc");
    heap.insert("ab");
    heap.insert("abcde");
    heap.insert("");

    REQUIRE(heap.peek()->get() == "abcde");

    std::vector<std::string> actual = drain(heap);
    std::vector<std::size_t> lengths;
    for (const auto& s : actual)
        lengths.push_back(s.size());

    REQUIRE(std::is_sorted(lengths.begin(), lengths.end(),
                           std::greater<std::size_t>{}));
}

// ---------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------

TEST_CASE("Move construction transfers ownership of the heap's contents",
          "[fibonacci_heap][move]")
{
    FibonacciHeap<int> original;
    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    FibonacciHeap<int> moved{std::move(original)};

    std::vector<int> actual = drain(moved);
    std::vector<int> expected{9, 5, 4, 3, 1, 1};
    REQUIRE(actual == expected);
}

TEST_CASE("Move assignment transfers ownership of the heap's contents",
          "[fibonacci_heap][move]")
{
    FibonacciHeap<int> original;
    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    FibonacciHeap<int> target;
    target.insert(-1);
    target = std::move(original);

    std::vector<int> actual = drain(target);
    std::vector<int> expected{9, 5, 4, 3, 1, 1};
    REQUIRE(actual == expected);
}

// ---------------------------------------------------------------------
// Stress tests
// ---------------------------------------------------------------------

TEST_CASE("Stress: many random insertions drain in sorted descending order",
          "[fibonacci_heap][stress]")
{
    constexpr std::size_t n = 5;
    std::vector<int> values = random_vector(n, 42);

    FibonacciHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end(), std::greater<int>{});

    REQUIRE(drain(heap) == expected);
}

TEST_CASE("Stress: interleaved insert/pop keeps the max-heap invariant",
          "[fibonacci_heap][stress]")
{
    std::mt19937 rng{7};
    std::uniform_int_distribution<int> value_dist{-10'000, 10'000};
    std::bernoulli_distribution op_dist{
        0.6}; // 60% insert, 40% pop when non-empty

    FibonacciHeap<int> heap;
    std::vector<int> reference; // kept sorted descending

    constexpr int operations = 20'000;
    for (int op = 0; op < operations; ++op) {
        bool do_insert = reference.empty() || op_dist(rng);
        if (do_insert) {
            int v = value_dist(rng);
            heap.insert(v);
            auto it = std::upper_bound(reference.begin(), reference.end(), v,
                                       std::greater<int>{});
            reference.insert(it, v);
        } else {
            REQUIRE(heap.peek() == reference.front());
            REQUIRE(heap.pop() == reference.front());
            reference.erase(reference.begin());
        }
    }

    REQUIRE(drain(heap) == reference);
}

TEST_CASE("Stress: heap drains fully leaving it empty and reusable",
          "[fibonacci_heap][stress]")
{
    FibonacciHeap<int> heap;
    std::vector<int> values = random_vector(10'000, 99);
    for (int v : values)
        heap.insert(v);

    (void)drain(heap);
    REQUIRE(!heap.peek());
    REQUIRE(!heap.pop());

    // heap should be usable again after being fully drained
    heap.insert(123);
    REQUIRE(heap.peek() == 123);
    REQUIRE(heap.pop() == 123);
}

// ---------------------------------------------------------------------
// increase_key
// ---------------------------------------------------------------------
//
// bool increase_key(const iterator& it, T new_value)
//
// `insert()` now returns an iterator identifying the inserted element,
// and it is that iterator -- rather than the element's value -- that is
// passed back in to `increase_key()`. This means the heap no longer needs
// to maintain its own value -> Node* map; the caller is responsible for
// hanging on to the iterator if they want to be able to increase that
// element's key later.
//
// increase_key() returns false if `new_value` does not represent an
// increase over the element's current value. Otherwise it updates the
// element in place and returns true. Passing an iterator that no longer
// refers to a live element in the heap (e.g. one that was already popped
// out) is undefined behaviour and is not exercised by these tests.

TEST_CASE("increase_key returns false when new_value does not increase value",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(5);
    auto it3 = heap.insert(3);
    auto it8 = heap.insert(8);
    heap.insert(1);
    heap.insert(9);

    SECTION("new_value equal to value")
    {
        REQUIRE(!heap.increase_key(it3, 3));
    }

    SECTION("new_value less than value")
    {
        REQUIRE(!heap.increase_key(it8, 2));
    }

    // heap contents/order should be untouched
    std::vector<int> actual = drain(heap);
    std::vector<int> expected{9, 8, 5, 3, 1};
    REQUIRE(actual == expected);
}

TEST_CASE("increase_key on the max element updates peek() and returns true",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(5);
    heap.insert(3);
    heap.insert(8);
    heap.insert(1);
    auto it9 = heap.insert(9);

    REQUIRE(heap.peek() == 9);
    REQUIRE(heap.increase_key(it9, 50));
    REQUIRE(heap.peek() == 50);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{50, 8, 5, 3, 1};
    REQUIRE(actual == expected);
}

TEST_CASE("increase_key on a non-max element can promote it to the new max",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(5);
    heap.insert(3);
    heap.insert(8);
    auto it1 = heap.insert(1);
    heap.insert(9);

    REQUIRE(heap.increase_key(it1, 100));
    REQUIRE(heap.peek() == 100);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{100, 9, 8, 5, 3};
    REQUIRE(actual == expected);
}

TEST_CASE("increase_key that does not change the max still reorders correctly",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(5);
    auto it3 = heap.insert(3);
    heap.insert(8);
    heap.insert(1);
    heap.insert(9);

    // 3 -> 6 is an increase but still well below the current max (9)
    REQUIRE(heap.increase_key(it3, 6));
    REQUIRE(heap.peek() == 9);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{9, 8, 6, 5, 1};
    REQUIRE(actual == expected);
}

TEST_CASE("increase_key affects only the element referred to by its "
          "iterator, even among duplicates",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(4);
    auto it = heap.insert(4);
    heap.insert(4);
    heap.insert(10);

    REQUIRE(heap.increase_key(it, 20));

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{20, 10, 4, 4};
    REQUIRE(actual == expected);
}

TEST_CASE("increase_key can be called repeatedly on the same iterator",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    auto it = heap.insert(1);
    heap.insert(2);

    REQUIRE(heap.increase_key(it, 5));
    REQUIRE(heap.peek() == 5);

    // `it` still refers to the same (now-updated) element, so increasing
    // it again should succeed.
    REQUIRE(heap.increase_key(it, 10));
    REQUIRE(heap.peek() == 10);

    // ...but "increasing" to a smaller value than its current one (10)
    // still fails.
    REQUIRE(!heap.increase_key(it, 3));
    REQUIRE(heap.peek() == 10);
}

TEST_CASE("increase_key works with a custom comparator",
          "[fibonacci_heap][increase_key][comparator]")
{
    // std::greater makes this a min-heap: "increasing" a key means moving
    // it closer to the front of the ordering, i.e. making it *smaller*.
    FibonacciHeap<int, std::greater<int>> heap;
    heap.insert(5);
    auto it3 = heap.insert(3);
    heap.insert(8);
    heap.insert(1);
    auto it9 = heap.insert(9);

    REQUIRE(heap.peek() == 1);

    SECTION("a value that moves toward the front returns true")
    {
        REQUIRE(heap.increase_key(it9, 0));
        REQUIRE(heap.peek() == 0);
    }

    SECTION("a value that moves away from the front returns false")
    {
        REQUIRE(!heap.increase_key(it3, 7));
    }
}

TEST_CASE("increase_key throws no surprises when reused after draining",
          "[fibonacci_heap][increase_key]")
{
    FibonacciHeap<int> heap;
    heap.insert(1);
    REQUIRE(heap.pop() == 1);

    // heap is now empty and should still be usable
    auto it1 = heap.insert(1);
    heap.insert(2);
    REQUIRE(heap.increase_key(it1, 3));
    REQUIRE(heap.peek() == 3);
}

TEST_CASE("Stress: interleaved insert/increase_key/pop keeps the max-heap "
          "invariant",
          "[fibonacci_heap][increase_key][stress]")
{
    std::mt19937 rng{123};
    std::uniform_int_distribution<int> value_dist{-10'000, 10'000};
    std::uniform_int_distribution<int> bump_dist{1, 100};
    std::uniform_int_distribution<int> op_dist{0, 9};

    using Heap = FibonacciHeap<int>;
    Heap heap;

    struct Entry {
        int value;
        Heap::iterator it;
    };
    std::vector<Entry> reference; // kept sorted descending by value

    constexpr int operations = 5'000;
    for (int op = 0; op < operations; ++op) {
        int choice = reference.empty() ? 0 : op_dist(rng);

        if (choice < 5) {
            // insert
            int v = value_dist(rng);
            auto it = heap.insert(v);
            auto pos = std::upper_bound(
                reference.begin(), reference.end(), v,
                [](int val, const Entry& e) { return val > e.value; });
            reference.insert(pos, Entry{v, it});
        } else if (choice < 8) {
            // increase_key on a random existing element
            std::uniform_int_distribution<std::size_t> idx_dist{
                0, reference.size() - 1};
            std::size_t idx = idx_dist(rng);
            int old_v = reference[idx].value;
            int new_v = old_v + bump_dist(rng);
            auto it = reference[idx].it;

            REQUIRE(heap.increase_key(it, new_v));
            reference.erase(reference.begin() +
                            static_cast<std::ptrdiff_t>(idx));
            auto pos = std::upper_bound(
                reference.begin(), reference.end(), new_v,
                [](int val, const Entry& e) { return val > e.value; });
            reference.insert(pos, Entry{new_v, it});
        } else {
            // pop
            REQUIRE(heap.peek() == reference.front().value);
            REQUIRE(heap.pop() == reference.front().value);
            reference.erase(reference.begin());
        }
    }

    std::vector<int> expected;
    expected.reserve(reference.size());
    for (const auto& e : reference)
        expected.push_back(e.value);

    REQUIRE(drain(heap) == expected);
}
