#include "binomial_heap.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace
{
    template<typename T, typename Compare>
    std::vector<T> drain(BinomialHeap<T, Compare>& heap)
    {
        std::vector<T> out;
        while (!heap.empty())
            out.push_back(heap.pop());

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

TEST_CASE("A default-constructed heap is empty", "[binomial_heap][basic]")
{
    BinomialHeap<int> heap;

    SECTION("peek() throws std::out_of_range")
    {
        REQUIRE_THROWS_AS(heap.peek(), std::out_of_range);
    }

    SECTION("pop() throws std::out_of_range")
    {
        REQUIRE_THROWS_AS(heap.pop(), std::out_of_range);
    }
}

// ---------------------------------------------------------------------
// Single-element behaviour
// ---------------------------------------------------------------------

TEST_CASE("Inserting a single element makes it the minimum",
          "[binomial_heap][basic]")
{
    BinomialHeap<int> heap;
    heap.insert(42);

    REQUIRE(heap.peek() == 42);

    SECTION("pop() removes it and leaves the heap empty")
    {
        REQUIRE(heap.pop() == 42);
        REQUIRE_THROWS_AS(heap.peek(), std::out_of_range);
        REQUIRE_THROWS_AS(heap.pop(), std::out_of_range);
    }
}

// ---------------------------------------------------------------------
// Multi-element ordering
// ---------------------------------------------------------------------

TEST_CASE("Heap always reports the minimum via peek()",
          "[binomial_heap][basic]")
{
    BinomialHeap<int> heap;

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
          "[binomial_heap][basic]")
{
    std::vector<int> values{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};

    BinomialHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(heap) == expected);
}

TEST_CASE("Heap handles duplicate values correctly", "[binomial_heap][basic]")
{
    BinomialHeap<int> heap;

    for (int i = 0; i < 5; ++i)
        heap.insert(7);

    for (int i = 0; i < 3; ++i)
        heap.insert(3);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{3, 3, 3, 7, 7, 7, 7, 7};

    REQUIRE(actual == expected);
}

TEST_CASE(
    "Heap correctly orders an arbitrary number of elements (power-of-two sizes)",
    "[binomial_heap][basic]")
{
    // Binomial heaps have interesting structural transitions at powers of two;
    // exercise sizes just below, at, and just above such boundaries.
    auto n = GENERATE(1u, 2u, 3u, 4u, 7u, 8u, 9u, 15u, 16u, 17u, 31u, 32u, 33u);
    CAPTURE(n);

    std::vector<int> values =
        random_vector(n, 1234u + static_cast<unsigned>(n));

    BinomialHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(heap) == expected);
}

// ---------------------------------------------------------------------
// Custom comparator
// ---------------------------------------------------------------------

TEST_CASE("A heap with std::greater<int> behaves as a max-heap",
          "[binomial_heap][comparator]")
{
    BinomialHeap<int, std::greater<int>> heap;

    for (int v : {5, 3, 8, 1, 9, 2})
        heap.insert(v);

    REQUIRE(heap.peek() == 9);

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{9, 8, 5, 3, 2, 1};

    REQUIRE(actual == expected);
}

struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const
    {
        return a.size() < b.size();
    }
};

TEST_CASE("A heap works with a non-trivial value type and custom comparator",
          "[binomial_heap][comparator]")
{
    BinomialHeap<std::string, ByLength> heap;

    heap.insert("a");
    heap.insert("abc");
    heap.insert("ab");
    heap.insert("abcde");
    heap.insert("");

    REQUIRE(heap.peek() == "");

    std::vector<std::string> actual = drain(heap);
    std::vector<std::size_t> lengths;

    for (const auto& s : actual)
        lengths.push_back(s.size());

    REQUIRE(std::is_sorted(lengths.begin(), lengths.end()));
}

// ---------------------------------------------------------------------
// merge()
// ---------------------------------------------------------------------

TEST_CASE("Merging with an empty heap is a no-op on contents",
          "[binomial_heap][merge]")
{
    BinomialHeap<int> heap;
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);

    BinomialHeap<int> empty;
    heap.merge(std::move(empty));

    std::vector<int> actual = drain(heap);
    std::vector<int> expected{1, 2, 3};

    REQUIRE(actual == expected);
}

TEST_CASE(
    "Merging an empty heap with a non-empty heap yields the non-empty heap's contents",
    "[binomial_heap][merge]")
{
    BinomialHeap<int> empty;
    BinomialHeap<int> other;

    other.insert(1);
    other.insert(2);
    other.insert(3);

    empty.merge(std::move(other));

    std::vector<int> actual = drain(empty);
    std::vector<int> expected{1, 2, 3};

    REQUIRE(actual == expected);
}

TEST_CASE(
    "Merging two non-empty heaps produces the union of their elements in order",
    "[binomial_heap][merge]")
{
    BinomialHeap<int> a;
    for (int v : {1, 4, 9, 16})
        a.insert(v);

    BinomialHeap<int> b;
    for (int v : {2, 3, 25, 0})
        b.insert(v);

    a.merge(std::move(b));

    std::vector<int> actual = drain(a);
    std::vector<int> expected{0, 1, 2, 3, 4, 9, 16, 25};

    REQUIRE(actual == expected);
}

TEST_CASE("Merging heaps of varying, mismatched sizes preserves total ordering",
          "[binomial_heap][merge]")
{
    auto size_a = GENERATE(0u, 1u, 5u, 17u, 64u);
    auto size_b = GENERATE(0u, 1u, 6u, 23u, 64u);
    CAPTURE(size_a, size_b);

    std::vector<int> values_a =
        random_vector(size_a, 111u + static_cast<unsigned>(size_a));
    std::vector<int> values_b =
        random_vector(size_b, 222u + static_cast<unsigned>(size_b));

    BinomialHeap<int> a;
    for (int v : values_a)
        a.insert(v);

    BinomialHeap<int> b;
    for (int v : values_b)
        b.insert(v);

    a.merge(std::move(b));

    std::vector<int> expected;
    expected.insert(expected.end(), values_a.begin(), values_a.end());
    expected.insert(expected.end(), values_b.begin(), values_b.end());
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(a) == expected);
}

// ---------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------

TEST_CASE("Move construction transfers ownership of the heap's contents",
          "[binomial_heap][move]")
{
    BinomialHeap<int> original;

    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    BinomialHeap<int> moved{std::move(original)};

    std::vector<int> actual = drain(moved);
    std::vector<int> expected{1, 1, 3, 4, 5, 9};

    REQUIRE(actual == expected);
}

TEST_CASE("Move assignment transfers ownership of the heap's contents",
          "[binomial_heap][move]")
{
    BinomialHeap<int> original;

    for (int v : {3, 1, 4, 1, 5, 9})
        original.insert(v);

    BinomialHeap<int> target;
    target.insert(-1);
    target = std::move(original);

    std::vector<int> actual = drain(target);
    std::vector<int> expected{1, 1, 3, 4, 5, 9};

    REQUIRE(actual == expected);
}

TEST_CASE("swap() exchanges the contents of two heaps", "[binomial_heap][move]")
{
    BinomialHeap<int> a;
    a.insert(1);
    a.insert(2);

    BinomialHeap<int> b;
    b.insert(100);
    b.insert(200);
    b.insert(300);

    a.swap(b);

    REQUIRE(a.peek() == 100);
    REQUIRE(b.peek() == 1);

    std::vector<int> a_contents = drain(a);
    std::vector<int> b_contents = drain(b);

    REQUIRE(a_contents == std::vector<int>{100, 200, 300});
    REQUIRE(b_contents == std::vector<int>{1, 2});
}

// ---------------------------------------------------------------------
// Stress tests
// ---------------------------------------------------------------------

TEST_CASE("Stress: many random insertions drain in sorted ascending order",
          "[binomial_heap][stress]")
{
    constexpr std::size_t n = 5;
    std::vector<int> values = random_vector(n, 42);

    BinomialHeap<int> heap;
    for (int v : values)
        heap.insert(v);

    std::vector<int> expected = values;
    std::sort(expected.begin(), expected.end());

    REQUIRE(drain(heap) == expected);
}

TEST_CASE("Stress: interleaved insert/pop keeps the min-heap invariant",
          "[binomial_heap][stress]")
{
    std::mt19937 rng{7};
    std::uniform_int_distribution<int> value_dist{-10'000, 10'000};
    std::bernoulli_distribution op_dist{
        0.6}; // 60% insert, 40% pop when non-empty

    BinomialHeap<int> heap;
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

TEST_CASE(
    "Stress: repeatedly merging many small heaps preserves all elements in order",
    "[binomial_heap][stress]")
{
    constexpr std::size_t num_heaps = 200;
    constexpr std::size_t per_heap = 100;

    BinomialHeap<int> combined;
    std::vector<int> all_values;

    for (std::size_t h = 0; h < num_heaps; ++h) {
        std::vector<int> values =
            random_vector(per_heap, static_cast<unsigned>(h) * 97u + 3u);

        BinomialHeap<int> small;
        for (int v : values)
            small.insert(v);

        combined.merge(std::move(small));
        all_values.insert(all_values.end(), values.begin(), values.end());
    }

    std::sort(all_values.begin(), all_values.end());

    REQUIRE(drain(combined) == all_values);
}

TEST_CASE("Stress: heap drains fully leaving it empty and reusable",
          "[binomial_heap][stress]")
{
    BinomialHeap<int> heap;

    std::vector<int> values = random_vector(10'000, 99);
    for (int v : values)
        heap.insert(v);

    (void)drain(heap);

    REQUIRE_THROWS_AS(heap.peek(), std::out_of_range);
    REQUIRE_THROWS_AS(heap.pop(), std::out_of_range);

    // Heap should be usable again after being fully drained.
    heap.insert(123);

    REQUIRE(heap.peek() == 123);
    REQUIRE(heap.pop() == 123);
}
