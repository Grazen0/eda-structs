#include "binary_heap.hpp"
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
    template<typename T, typename Compare>
    std::vector<T> drain(BinaryHeap<T, Compare>& heap)
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

TEST_CASE("A default-constructed heap is empty", "[binary_heap][basic]")
{
    BinaryHeap<int> heap;

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

TEST_CASE("Inserting a single element makes it the minimum",
          "[binary_heap][basic]")
{
    BinaryHeap<int> heap;
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

TEST_CASE("Heap always reports the minimum via peek()", "[binary_heap][basic]")
{
    BinaryHeap<int> heap;

    heap.insert(5);
    REQUIRE(heap.peek() == 5);

    heap.insert(10);
    REQUIRE(heap.peek() == 5);

    heap.insert(1);
    REQUIRE(heap.peek() == 1);

    heap.insert(20);
    REQUIRE(heap.peek() == 1);

    heap.insert(0);
    REQUIRE(heap.peek() == 0);
}

TEST_CASE("Popping repeatedly yields elements in ascending order",
          "[binary_heap][basic]")
{
    std::vector<int> values{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};

    BinaryHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(heap) == expected);
}

TEST_CASE("Heap handles duplicate values correctly", "[binary_heap][basic]")
{
    BinaryHeap<int> heap;

    for (int i = 0; i < 5; ++i)
        heap.insert(7);

    for (int i = 0; i < 3; ++i)
        heap.insert(3);

    std::vector<int> expected{3, 3, 3, 7, 7, 7, 7, 7};
    REQUIRE(drain(heap) == expected);
}

// ---------------------------------------------------------------------
// Custom comparator
// ---------------------------------------------------------------------

TEST_CASE("A heap with std::greater<int> behaves as a max-heap",
          "[binary_heap][comparator]")
{
    BinaryHeap<int, std::greater<int>> heap;

    for (int v : {5, 3, 8, 1, 9, 2})
        heap.insert(v);

    REQUIRE(heap.peek() == 9);

    std::vector<int> expected{9, 8, 5, 3, 2, 1};
    REQUIRE(drain(heap) == expected);
}

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const
    {
        return a.size() < b.size();
    }
};

TEST_CASE("A heap works with a non-trivial value type and custom comparator",
          "[binary_heap][comparator]")
{
    BinaryHeap<std::string, ByLength> heap;

    heap.insert("a");
    heap.insert("abc");
    heap.insert("ab");
    heap.insert("abcde");
    heap.insert("");

    REQUIRE(heap.peek()->get() == "");

    std::vector<std::string> actual = drain(heap);
    std::vector<std::size_t> lengths;

    for (const auto& s : actual)
        lengths.push_back(s.size());

    REQUIRE(std::is_sorted(lengths.begin(), lengths.end()));
}

// ---------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------

TEST_CASE("Move construction transfers ownership of the heap's contents",
          "[binary_heap][move]")
{
    BinaryHeap<int> original;

    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    BinaryHeap<int> moved{std::move(original)};

    std::vector<int> expected{1, 1, 3, 4, 5, 9};
    REQUIRE(drain(moved) == expected);
}

TEST_CASE("Move assignment transfers ownership of the heap's contents",
          "[binary_heap][move]")
{
    BinaryHeap<int> original;

    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    BinaryHeap<int> target;
    target.insert(-1);
    target = std::move(original);

    std::vector<int> expected{1, 1, 3, 4, 5, 9};
    REQUIRE(drain(target) == expected);
}

// ---------------------------------------------------------------------
// Stress tests
// ---------------------------------------------------------------------

TEST_CASE("Stress: many random insertions drain in sorted ascending order",
          "[binary_heap][stress]")
{
    constexpr std::size_t n = 5;
    std::vector<int> values = random_vector(n, 42);

    BinaryHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(heap) == expected);
}

TEST_CASE("Stress: interleaved insert/pop keeps the min-heap invariant",
          "[binary_heap][stress]")
{
    std::mt19937 rng{7};
    std::uniform_int_distribution<int> value_dist{-10'000, 10'000};
    std::bernoulli_distribution op_dist{
        0.6}; // 60% insert, 40% pop when non-empty

    BinaryHeap<int> heap;
    std::vector<int> reference; // kept sorted ascending

    constexpr int operations = 20'000;

    for (int op = 0; op < operations; ++op) {
        bool do_insert = reference.empty() || op_dist(rng);

        if (do_insert) {
            int v = value_dist(rng);
            heap.insert(v);

            auto it = std::upper_bound(reference.begin(), reference.end(), v);
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
          "[binary_heap][stress]")
{
    BinaryHeap<int> heap;

    std::vector<int> values = random_vector(10'000, 99);
    for (int v : values)
        heap.insert(v);

    (void)drain(heap);

    REQUIRE(!heap.peek());
    REQUIRE(!heap.pop());

    // Heap should be usable again after being fully drained.
    heap.insert(123);

    REQUIRE(heap.peek() == 123);
    REQUIRE(heap.pop() == 123);
}
