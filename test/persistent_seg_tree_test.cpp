#include "persistent_seg_tree.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <limits>
#include <stdexcept>

TEST_CASE("persistent segment tree basic queries", "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3, 4, 5});

    REQUIRE(tree.size() == 5);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 4) == 15);
    REQUIRE(tree.query(v0, 1, 3) == 9);

    for (std::size_t i = 0; i < tree.size(); ++i)
        REQUIRE(tree.query(v0, i, i) == static_cast<int>(i + 1));

    REQUIRE(tree.query(v0, 0, 0) == 1);
    REQUIRE(tree.query(v0, 4, 4) == 5);
    REQUIRE(tree.query(v0, 2, 2) == 3);
}

TEST_CASE("persistent segment tree can be initialized with a size",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, 8);

    REQUIRE(tree.size() == 8);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 7) == 0);

    for (std::size_t i = 0; i < tree.size(); ++i)
        REQUIRE(tree.query(v0, i, i) == 0);
}

TEST_CASE("persistent segment tree size initialization supports updates",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, 5);

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 0, 10);
    auto v2 = tree.update(v1, 2, 20);
    auto v3 = tree.update(v2, 4, 30);

    REQUIRE(tree.query(v0, 0, 4) == 0);

    REQUIRE(tree.query(v1, 0, 4) == 10);
    REQUIRE(tree.query(v1, 0, 0) == 10);

    REQUIRE(tree.query(v2, 0, 4) == 30);
    REQUIRE(tree.query(v2, 0, 2) == 30);

    REQUIRE(tree.query(v3, 0, 4) == 60);
    REQUIRE(tree.query(v3, 0, 4) == 60);
}

TEST_CASE("persistent segment tree updates create new versions",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3, 4, 5});

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 2, 100);

    REQUIRE(tree.query(v0, 0, 4) == 15);
    REQUIRE(tree.query(v0, 2, 2) == 3);

    REQUIRE(tree.query(v1, 0, 4) == 112);
    REQUIRE(tree.query(v1, 2, 2) == 100);

    REQUIRE(tree.query(v1, 0, 1) == 3);
    REQUIRE(tree.query(v1, 3, 4) == 9);
}

TEST_CASE("persistent segment tree supports branching versions",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3, 4});

    auto v0 = tree.init();

    auto v1 = tree.update(v0, 0, 10);
    auto v2 = tree.update(v0, 3, 40);

    REQUIRE(tree.query(v0, 0, 3) == 10);

    REQUIRE(tree.query(v1, 0, 3) == 19);
    REQUIRE(tree.query(v1, 0, 0) == 10);
    REQUIRE(tree.query(v1, 3, 3) == 4);

    REQUIRE(tree.query(v2, 0, 3) == 46);
    REQUIRE(tree.query(v2, 0, 0) == 1);
    REQUIRE(tree.query(v2, 3, 3) == 40);
}

TEST_CASE("persistent segment tree supports multiple successive versions",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3, 4});

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 0, 10);
    auto v2 = tree.update(v1, 1, 20);
    auto v3 = tree.update(v2, 2, 30);
    auto v4 = tree.update(v3, 3, 40);

    REQUIRE(tree.query(v0, 0, 3) == 10);
    REQUIRE(tree.query(v1, 0, 3) == 19);
    REQUIRE(tree.query(v2, 0, 3) == 37);
    REQUIRE(tree.query(v3, 0, 3) == 64);
    REQUIRE(tree.query(v4, 0, 3) == 100);

    REQUIRE(tree.query(v0, 0, 0) == 1);
    REQUIRE(tree.query(v1, 0, 0) == 10);
    REQUIRE(tree.query(v2, 1, 1) == 20);
    REQUIRE(tree.query(v3, 2, 2) == 30);
    REQUIRE(tree.query(v4, 3, 3) == 40);
}

TEST_CASE(
    "persistent segment tree supports updating the same position repeatedly",
    "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3});

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 1, 10);
    auto v2 = tree.update(v1, 1, 20);
    auto v3 = tree.update(v2, 1, -5);

    REQUIRE(tree.query(v0, 1, 1) == 2);
    REQUIRE(tree.query(v1, 1, 1) == 10);
    REQUIRE(tree.query(v2, 1, 1) == 20);
    REQUIRE(tree.query(v3, 1, 1) == -5);

    REQUIRE(tree.query(v0, 0, 2) == 6);
    REQUIRE(tree.query(v1, 0, 2) == 14);
    REQUIRE(tree.query(v2, 0, 2) == 24);
    REQUIRE(tree.query(v3, 0, 2) == -1);
}

TEST_CASE("persistent segment tree supports updates at boundaries",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {1, 2, 3, 4, 5});

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 0, 100);
    auto v2 = tree.update(v1, tree.size() - 1, 200);

    REQUIRE(tree.query(v0, 0, 0) == 1);
    REQUIRE(tree.query(v0, 4, 4) == 5);

    REQUIRE(tree.query(v1, 0, 0) == 100);
    REQUIRE(tree.query(v1, 4, 4) == 5);

    REQUIRE(tree.query(v2, 0, 0) == 100);
    REQUIRE(tree.query(v2, 4, 4) == 200);
    REQUIRE(tree.query(v2, 0, 4) == 309);
}

TEST_CASE("persistent segment tree works with a minimum operator",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::function<int(int, int)>> tree(
        std::numeric_limits<int>::max(), {7, 3, 9, 1, 5},
        [](int a, int b) { return std::min(a, b); });

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 4) == 1);
    REQUIRE(tree.query(v0, 0, 1) == 3);
    REQUIRE(tree.query(v0, 2, 4) == 1);
    REQUIRE(tree.query(v0, 0, 0) == 7);

    auto v1 = tree.update(v0, 3, 10);

    REQUIRE(tree.query(v0, 0, 4) == 1);
    REQUIRE(tree.query(v1, 0, 4) == 3);
}

TEST_CASE("persistent segment tree works with a maximum operator",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::function<int(int, int)>> tree(
        std::numeric_limits<int>::lowest(), {7, 3, 9, 1, 5},
        [](int a, int b) { return std::max(a, b); });

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 4) == 9);
    REQUIRE(tree.query(v0, 0, 1) == 7);
    REQUIRE(tree.query(v0, 3, 4) == 5);

    auto v1 = tree.update(v0, 2, -1);

    REQUIRE(tree.query(v0, 0, 4) == 9);
    REQUIRE(tree.query(v1, 0, 4) == 7);
}

TEST_CASE("persistent segment tree works with multiplication",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::multiplies<int>> tree(1, {2, 3, 4, 5});

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 3) == 120);
    REQUIRE(tree.query(v0, 1, 2) == 12);
    REQUIRE(tree.query(v0, 2, 2) == 4);

    auto v1 = tree.update(v0, 1, 10);

    REQUIRE(tree.query(v0, 0, 3) == 120);
    REQUIRE(tree.query(v1, 0, 3) == 400);
}

TEST_CASE("persistent segment tree works with a non-commutative operator",
          "[persistent_seg_tree]")
{
    auto concat = [](const std::string& a, const std::string& b) {
        return a + b;
    };

    PersistentSegTree<std::string, decltype(concat)> tree(
        "", {"a", "b", "c", "d"}, concat);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 3) == "abcd");
    REQUIRE(tree.query(v0, 1, 2) == "bc");
    REQUIRE(tree.query(v0, 1, 3) == "bcd");

    auto v1 = tree.update(v0, 1, std::string{"X"});

    REQUIRE(tree.query(v0, 0, 3) == "abcd");
    REQUIRE(tree.query(v1, 0, 3) == "aXcd");
    REQUIRE(tree.query(v1, 1, 3) == "Xcd");
}

TEST_CASE("persistent segment tree supports a single element",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>> tree(0, {42});

    REQUIRE(tree.size() == 1);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 0) == 42);

    auto v1 = tree.update(v0, 0, 100);

    REQUIRE(tree.query(v0, 0, 0) == 42);
    REQUIRE(tree.query(v1, 0, 0) == 100);
}

TEST_CASE("persistent segment tree rejects zero size construction",
          "[persistent_seg_tree]")
{
    REQUIRE_THROWS_AS((PersistentSegTree<int, std::plus<int>>(0, 0)),
                      std::invalid_argument);
}

TEST_CASE("persistent segment tree rejects empty initializer list",
          "[persistent_seg_tree]")
{
    REQUIRE_THROWS_AS((PersistentSegTree<int, std::plus<int>>(0, {})),
                      std::invalid_argument);
}

TEST_CASE("persistent segment tree can be initialized from an iterator range",
          "[persistent_seg_tree]")
{
    std::vector<int> data{1, 2, 3, 4, 5};

    PersistentSegTree<int, std::plus<int>> tree(0, data.begin(), data.end());

    REQUIRE(tree.size() == data.size());

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 4) == 15);
    REQUIRE(tree.query(v0, 1, 3) == 9);

    for (std::size_t i = 0; i < tree.size(); ++i)
        REQUIRE(tree.query(v0, i, i) == data[i]);
}

TEST_CASE("persistent segment tree iterator constructor respects subranges",
          "[persistent_seg_tree]")
{
    std::vector<int> data{10, 20, 1, 2, 3, 40};

    PersistentSegTree<int, std::plus<int>> tree(0, data.begin() + 2,
                                                data.begin() + 5);

    REQUIRE(tree.size() == 3);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 2) == 6);
    REQUIRE(tree.query(v0, 0, 0) == 1);
    REQUIRE(tree.query(v0, 1, 1) == 2);
    REQUIRE(tree.query(v0, 2, 2) == 3);
}

TEST_CASE("persistent segment tree iterator constructor supports updates",
          "[persistent_seg_tree]")
{
    std::vector<int> data{1, 2, 3, 4};

    PersistentSegTree<int, std::plus<int>> tree(0, data.begin(), data.end());

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 1, 100);

    REQUIRE(tree.query(v0, 0, 3) == 10);
    REQUIRE(tree.query(v0, 1, 1) == 2);

    REQUIRE(tree.query(v1, 0, 3) == 108);
    REQUIRE(tree.query(v1, 1, 1) == 100);
}

TEST_CASE(
    "persistent segment tree iterator constructor works with non-vector iterators",
    "[persistent_seg_tree]")
{
    std::vector<int> data{5, 10, 15};

    // Uses const_iterators rather than mutable iterators.
    PersistentSegTree<int, std::plus<int>> tree(0, data.cbegin(), data.cend());

    REQUIRE(tree.size() == 3);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 2) == 30);
    REQUIRE(tree.query(v0, 1, 2) == 25);
}

TEST_CASE(
    "persistent segment tree iterator constructor supports custom operators",
    "[persistent_seg_tree]")
{
    std::vector<int> data{7, 3, 9, 1, 5};

    auto minimum = [](int a, int b) { return std::min(a, b); };

    PersistentSegTree<int, decltype(minimum)> tree(
        std::numeric_limits<int>::max(), data.begin(), data.end(), minimum);

    auto v0 = tree.init();

    REQUIRE(tree.size() == data.size());
    REQUIRE(tree.query(v0, 0, 4) == 1);
    REQUIRE(tree.query(v0, 0, 1) == 3);
    REQUIRE(tree.query(v0, 2, 4) == 1);
}

TEST_CASE(
    "persistent segment tree iterator constructor supports a single element",
    "[persistent_seg_tree]")
{
    std::vector<int> data{42};

    PersistentSegTree<int, std::plus<int>> tree(0, data.begin(), data.end());

    REQUIRE(tree.size() == 1);

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 0) == 42);
}

TEST_CASE("persistent segment tree iterator constructor rejects an empty range",
          "[persistent_seg_tree]")
{
    std::vector<int> data;

    REQUIRE_THROWS_AS(
        (PersistentSegTree<int, std::plus<int>>(0, data.begin(), data.end())),
        std::invalid_argument);
}
