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
