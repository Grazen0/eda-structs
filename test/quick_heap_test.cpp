#include "quick_heap.hpp"
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <string>

TEST_CASE("new heap is empty", "[quick_heap]")
{
    QuickHeap<int> heap;

    REQUIRE(heap.empty());
    REQUIRE(heap.size() == 0);
    REQUIRE(heap.peek() == std::nullopt);
    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("insert adds elements", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(3);
    heap.insert(1);
    heap.insert(2);

    REQUIRE_FALSE(heap.empty());
    REQUIRE(heap.size() == 3);
}

TEST_CASE("pop returns elements in sorted order", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(5);
    heap.insert(1);
    heap.insert(4);
    heap.insert(2);
    heap.insert(3);

    REQUIRE(*heap.pop() == 1);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 5);
    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("peek returns the next element without removing it", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(5);
    heap.insert(1);
    heap.insert(3);

    REQUIRE(heap.peek().has_value());
    REQUIRE(heap.peek()->get() == 1);

    REQUIRE(heap.size() == 3);

    REQUIRE(heap.pop() == 1);
    REQUIRE(heap.peek()->get() == 3);
}

TEST_CASE("duplicate values are handled", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(3);
    heap.insert(1);
    heap.insert(3);
    heap.insert(2);
    heap.insert(1);
    heap.insert(3);

    REQUIRE(heap.pop() == 1);
    REQUIRE(heap.pop() == 1);
    REQUIRE(heap.pop() == 2);
    REQUIRE(heap.pop() == 3);
    REQUIRE(heap.pop() == 3);
    REQUIRE(heap.pop() == 3);
    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("initializer list constructs heap", "[quick_heap]")
{
    QuickHeap<int> heap{5, 1, 4, 2, 3};

    REQUIRE(heap.size() == 5);

    REQUIRE(*heap.pop() == 1);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 5);
}

TEST_CASE("single element heap", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(42);

    REQUIRE(heap.size() == 1);
    REQUIRE_FALSE(heap.empty());

    REQUIRE(heap.peek()->get() == 42);
    REQUIRE(heap.pop() == 42);

    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("elements can be inserted after popping", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(3);
    heap.insert(1);
    heap.insert(2);

    REQUIRE(heap.pop() == 1);

    heap.insert(0);

    REQUIRE(heap.pop() == 0);
    REQUIRE(heap.pop() == 2);
    REQUIRE(heap.pop() == 3);
    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("custom comparator produces max heap behavior", "[quick_heap]")
{
    QuickHeap<int, std::greater<int>> heap;

    heap.insert(1);
    heap.insert(5);
    heap.insert(3);
    heap.insert(2);
    heap.insert(4);

    REQUIRE(*heap.pop() == 5);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 1);
}

TEST_CASE("emplace constructs values", "[quick_heap]")
{
    QuickHeap<std::string> heap;

    heap.emplace("banana");
    heap.emplace("apple");
    heap.emplace("cherry");

    REQUIRE(heap.pop() == "apple");
    REQUIRE(heap.pop() == "banana");
    REQUIRE(heap.pop() == "cherry");
}

TEST_CASE("heap works with already sorted input", "[quick_heap]")
{
    QuickHeap<int> heap{1, 2, 3, 4, 5};

    REQUIRE(*heap.pop() == 1);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 5);
}

TEST_CASE("heap works with reverse sorted input", "[quick_heap]")
{
    QuickHeap<int> heap{5, 4, 3, 2, 1};

    REQUIRE(*heap.pop() == 1);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 5);
}

TEST_CASE("heap handles negative values", "[quick_heap]")
{
    QuickHeap<int> heap;

    heap.insert(-5);
    heap.insert(10);
    heap.insert(-1);
    heap.insert(0);
    heap.insert(-10);

    REQUIRE(*heap.pop() == -10);
    REQUIRE(*heap.pop() == -5);
    REQUIRE(*heap.pop() == -1);
    REQUIRE(*heap.pop() == 0);
    REQUIRE(*heap.pop() == 10);
}

TEST_CASE("large number of elements", "[quick_heap]")
{
    QuickHeap<int> heap;

    constexpr int count = 1000;

    for (int i = count - 1; i >= 0; --i)
        heap.insert(i);

    REQUIRE(heap.size() == count);

    for (int i = 0; i < count; ++i)
        REQUIRE(*heap.pop() == i);

    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("iterator range constructs heap", "[quick_heap]")
{
    std::vector<int> data{5, 1, 4, 2, 3};
    QuickHeap<int> heap(data.begin(), data.end());

    REQUIRE(heap.size() == data.size());
    REQUIRE(*heap.pop() == 1);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 5);
    REQUIRE(heap.pop() == std::nullopt);
}

TEST_CASE("iterator range constructs heap with custom comparator",
          "[quick_heap]")
{
    std::vector<int> data{5, 1, 4, 2, 3};
    QuickHeap<int, std::greater<int>> heap(data.begin(), data.end());

    REQUIRE(heap.size() == data.size());
    REQUIRE(*heap.pop() == 5);
    REQUIRE(*heap.pop() == 4);
    REQUIRE(*heap.pop() == 3);
    REQUIRE(*heap.pop() == 2);
    REQUIRE(*heap.pop() == 1);
    REQUIRE(heap.pop() == std::nullopt);
}
