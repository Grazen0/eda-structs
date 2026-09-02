#include "persistent_seg_tree.hpp"
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <limits>
#include <numeric>
#include <vector>

TEST_CASE("persistent segment tree basic queries", "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {1, 2, 3, 4, 5};

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

TEST_CASE("persistent segment tree update changes only new version",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {1, 2, 3, 4, 5};

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 0, 10);

    // Original version is unchanged.
    REQUIRE(tree.query(v0, 0, 4) == 15);
    REQUIRE(tree.query(v0, 0, 0) == 1);

    // New version contains the update.
    REQUIRE(tree.query(v1, 0, 4) == 24);
    REQUIRE(tree.query(v1, 0, 0) == 10);

    // Unrelated ranges are unchanged.
    REQUIRE(tree.query(v1, 1, 3) == 9);
    REQUIRE(tree.query(v1, 4, 4) == 5);
}

TEST_CASE("persistent segment tree supports branching versions",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {1, 2, 3, 4, 5};

    auto v0 = tree.init();

    auto v1 = tree.update(v0, 0, 10);
    auto v2 = tree.update(v0, 4, 50);

    // Both versions originate from v0 and must not affect one another.
    REQUIRE(tree.query(v0, 0, 4) == 15);

    REQUIRE(tree.query(v1, 0, 4) == 24);
    REQUIRE(tree.query(v1, 0, 0) == 10);
    REQUIRE(tree.query(v1, 4, 4) == 5);

    REQUIRE(tree.query(v2, 0, 4) == 60);
    REQUIRE(tree.query(v2, 0, 0) == 1);
    REQUIRE(tree.query(v2, 4, 4) == 50);

    // Branch from an already modified version.
    auto v3 = tree.update(v1, 2, 30);

    REQUIRE(tree.query(v3, 0, 4) == 51);
    REQUIRE(tree.query(v3, 0, 0) == 10);
    REQUIRE(tree.query(v3, 2, 2) == 30);

    // v1 is still unchanged.
    REQUIRE(tree.query(v1, 0, 4) == 24);
    REQUIRE(tree.query(v1, 2, 2) == 3);
}

TEST_CASE("persistent segment tree handles repeated updates at same position",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {1, 2, 3, 4};

    auto v0 = tree.init();
    auto v1 = tree.update(v0, 1, 20);
    auto v2 = tree.update(v1, 1, 30);
    auto v3 = tree.update(v2, 1, -10);

    REQUIRE(tree.query(v0, 0, 3) == 10);
    REQUIRE(tree.query(v1, 0, 3) == 28);
    REQUIRE(tree.query(v2, 0, 3) == 38);
    REQUIRE(tree.query(v3, 0, 3) == -2);

    REQUIRE(tree.query(v0, 1, 1) == 2);
    REQUIRE(tree.query(v1, 1, 1) == 20);
    REQUIRE(tree.query(v2, 1, 1) == 30);
    REQUIRE(tree.query(v3, 1, 1) == -10);
}

TEST_CASE("persistent segment tree handles negative values",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {-10, 5, -3, 8, -20, 7};

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 5) == -13);
    REQUIRE(tree.query(v0, 0, 2) == -8);
    REQUIRE(tree.query(v0, 2, 4) == -15);
    REQUIRE(tree.query(v0, 4, 5) == -13);
    REQUIRE(tree.query(v0, 1, 1) == 5);
}

TEST_CASE("persistent segment tree supports multiplication",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::multiplies<int>, 1> tree = {2, 3, 4, 5};

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 3) == 120);
    REQUIRE(tree.query(v0, 0, 1) == 6);
    REQUIRE(tree.query(v0, 1, 2) == 12);
    REQUIRE(tree.query(v0, 2, 3) == 20);

    auto v1 = tree.update(v0, 1, 10);

    REQUIRE(tree.query(v0, 0, 3) == 120);
    REQUIRE(tree.query(v1, 0, 3) == 400);
    REQUIRE(tree.query(v1, 1, 1) == 10);
}

TEST_CASE("persistent segment tree supports minimum", "[persistent_seg_tree]")
{
    using Tree = PersistentSegTree<int, std::function<int(int, int)>,
                                   std::numeric_limits<int>::max()>;

    Tree tree({8, 3, 7, 1, 9, 4, 6},
              [](int a, int b) { return std::min(a, b); });

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 6) == 1);
    REQUIRE(tree.query(v0, 0, 2) == 3);
    REQUIRE(tree.query(v0, 2, 5) == 1);
    REQUIRE(tree.query(v0, 4, 6) == 4);

    auto v1 = tree.update(v0, 2, -100);

    REQUIRE(tree.query(v0, 0, 6) == 1);
    REQUIRE(tree.query(v1, 0, 6) == -100);
    REQUIRE(tree.query(v1, 3, 6) == 1);
}

TEST_CASE("persistent segment tree supports maximum", "[persistent_seg_tree]")
{
    using Tree = PersistentSegTree<int, std::function<int(int, int)>,
                                   std::numeric_limits<int>::lowest()>;

    Tree tree({8, 3, 7, 1, 9, 4, 6},
              [](int a, int b) { return std::max(a, b); });

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 6) == 9);
    REQUIRE(tree.query(v0, 0, 2) == 8);
    REQUIRE(tree.query(v0, 2, 5) == 9);
    REQUIRE(tree.query(v0, 4, 6) == 9);

    auto v1 = tree.update(v0, 3, 100);

    REQUIRE(tree.query(v0, 0, 6) == 9);
    REQUIRE(tree.query(v1, 0, 6) == 100);
    REQUIRE(tree.query(v1, 0, 2) == 8);
}

TEST_CASE("persistent segment tree handles non-power-of-two sizes",
          "[persistent_seg_tree]")
{
    // Deliberately use a size that isn't a power of two.
    PersistentSegTree<int, std::plus<int>, 0> tree = {1, 2, 3, 4, 5,
                                                      6, 7, 8, 9};

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 8) == 45);
    REQUIRE(tree.query(v0, 0, 3) == 10);
    REQUIRE(tree.query(v0, 4, 8) == 35);
    REQUIRE(tree.query(v0, 3, 7) == 30);
    REQUIRE(tree.query(v0, 7, 8) == 17);

    auto v1 = tree.update(v0, 8, 100);

    REQUIRE(tree.query(v0, 0, 8) == 45);
    REQUIRE(tree.query(v1, 0, 8) == 136);
    REQUIRE(tree.query(v1, 8, 8) == 100);
}

TEST_CASE("persistent segment tree handles a large number of updates",
          "[persistent_seg_tree][stress]")
{
    // A reasonably large tree while keeping the initializer-list constructor
    // practical.
    PersistentSegTree<int, std::plus<int>, 0> tree = {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    };

    constexpr std::size_t N = 64;
    constexpr std::size_t UPDATES = 20'000;

    auto v = tree.init();

    std::vector<int> expected(N);
    std::iota(expected.begin(), expected.end(), 0);

    std::vector<PersistentSegTree<int, std::plus<int>, 0>::Version> versions;
    versions.reserve(UPDATES + 1);
    versions.push_back(v);

    for (std::size_t i = 0; i < UPDATES; ++i) {
        const std::size_t pos = (i * 37) % N;
        const int value = static_cast<int>((i * 13) % 1000) - 500;

        expected[pos] = value;
        v = tree.update(v, pos, value);
        versions.push_back(v);

        // Periodically check the entire current tree.
        if (i % 500 == 0) {
            const int expected_sum =
                std::accumulate(expected.begin(), expected.end(), 0);

            REQUIRE(tree.query(v, 0, N - 1) == expected_sum);
        }
    }

    const int expected_sum =
        std::accumulate(expected.begin(), expected.end(), 0);

    REQUIRE(tree.query(v, 0, N - 1) == expected_sum);

    // Check every individual element in the final version.
    for (std::size_t i = 0; i < N; ++i)
        REQUIRE(tree.query(v, i, i) == expected[i]);
}

TEST_CASE("persistent segment tree stress-tests independent branches",
          "[persistent_seg_tree][stress]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    };

    constexpr std::size_t N = 32;
    constexpr std::size_t BRANCHES = 2'000;

    auto root = tree.init();

    std::vector<PersistentSegTree<int, std::plus<int>, 0>::Version> versions;
    versions.reserve(BRANCHES);

    // Every update is made from the original root. This catches accidental
    // mutation of existing nodes.
    for (std::size_t i = 0; i < BRANCHES; ++i) {
        const std::size_t pos = (i * 11) % N;
        const int value = static_cast<int>(i * 7);

        auto version = tree.update(root, pos, value);
        versions.push_back(version);

        REQUIRE(tree.query(root, 0, N - 1) == 528);

        REQUIRE(tree.query(version, pos, pos) == value);
    }

    // Re-check a selection of old versions after all later updates.
    for (std::size_t i = 0; i < BRANCHES; i += 37) {
        const std::size_t pos = (i * 11) % N;
        const int value = static_cast<int>(i * 7);

        REQUIRE(tree.query(versions[i], pos, pos) == value);
        REQUIRE(tree.query(root, 0, N - 1) == 528);
    }
}

TEST_CASE("persistent segment tree handles identity through partial queries",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> tree = {10, 20, 30, 40,
                                                      50, 60, 70};

    auto v0 = tree.init();

    REQUIRE(tree.query(v0, 0, 6) == 280);

    // These ranges force recursive queries where some branches are completely
    // outside the requested interval and must contribute IDENTITY.
    REQUIRE(tree.query(v0, 1, 1) == 20);
    REQUIRE(tree.query(v0, 1, 2) == 50);
    REQUIRE(tree.query(v0, 2, 4) == 120);
    REQUIRE(tree.query(v0, 3, 5) == 150);
    REQUIRE(tree.query(v0, 5, 6) == 130);
}

TEST_CASE("persistent segment tree move construction and assignment",
          "[persistent_seg_tree]")
{
    PersistentSegTree<int, std::plus<int>, 0> original = {1, 2, 3, 4};

    auto v = original.update(original.init(), 1, 20);

    PersistentSegTree<int, std::plus<int>, 0> moved(std::move(original));

    REQUIRE(moved.size() == 4);
    REQUIRE(moved.query(v, 0, 3) == 28);

    PersistentSegTree<int, std::plus<int>, 0> assigned = {100, 200};

    assigned = std::move(moved);

    REQUIRE(assigned.size() == 4);
    REQUIRE(assigned.query(v, 0, 3) == 28);
}

TEST_CASE("size constructor works with minimum operator",
          "[persistent_seg_tree]")
{
    using Tree = PersistentSegTree<int, std::function<int(int, int)>,
                                   std::numeric_limits<int>::max()>;

    Tree tree(16, [](int a, int b) { return std::min(a, b); });

    auto v0 = tree.init();

    // All elements are int{}, i.e. 0.
    REQUIRE(tree.query(v0, 0, 15) == 0);

    auto v1 = tree.update(v0, 7, -100);

    REQUIRE(tree.query(v0, 0, 15) == 0);
    REQUIRE(tree.query(v1, 0, 15) == -100);
    REQUIRE(tree.query(v1, 0, 6) == 0);
    REQUIRE(tree.query(v1, 8, 15) == 0);
}
