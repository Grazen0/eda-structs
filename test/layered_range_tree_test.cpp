#include "layered_range_tree.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <random>
#include <utility>
#include <vector>

namespace
{

    // Brute-force 2D range count, used as an oracle to check the tree against.
    template<typename T>
    std::size_t brute_force_query(const std::vector<std::pair<T, T>>& pts, T l1,
                                  T r1, T l2, T r2)
    {
        if (l1 > r1 || l2 > r2)
            return 0;

        std::size_t count = 0;
        for (const auto& [x, y] : pts) {
            if (x >= l1 && x <= r1 && y >= l2 && y <= r2)
                ++count;
        }
        return count;
    }

} // namespace

TEST_CASE("single point tree", "[layered_range_tree]")
{
    LayeredRangeTree<int> tree{
        {5, 10}
    };

    REQUIRE(tree.size() == 1);
    REQUIRE_FALSE(tree.empty());

    REQUIRE(tree.query(0, 10, 0, 20) == 1);
    REQUIRE(tree.query(5, 5, 10, 10) == 1);
    REQUIRE(tree.query(6, 10, 0, 20) == 0);
    REQUIRE(tree.query(0, 4, 0, 20) == 0);
    REQUIRE(tree.query(0, 10, 11, 20) == 0);
    REQUIRE(tree.query(0, 10, 0, 9) == 0);
}

TEST_CASE("small fixed point set matches expected counts",
          "[layered_range_tree]")
{
    // Sorted by (x, y) lexicographically, as required by the constructor.
    LayeredRangeTree<int> tree{
        {1, 5},
        {2, 3},
        {2, 8},
        {4, 1},
        {4, 9},
        {6, 4},
        {8, 2},
        {8, 7}
    };

    REQUIRE(tree.size() == 8);
    REQUIRE_FALSE(tree.empty());

    // Full range covers everything.
    REQUIRE(tree.query(1, 8, 1, 9) == 8);
    REQUIRE(tree.query(-100, 100, -100, 100) == 8);

    // Narrow slices.
    REQUIRE(tree.query(2, 2, 0, 10) == 2); // x == 2: (2,3),(2,8)
    REQUIRE(tree.query(1, 4, 1, 5) == 3); // (1,5),(2,3),(4,1)
    REQUIRE(tree.query(5, 8, 0, 10) == 3); // (6,4),(8,2),(8,7)
    REQUIRE(tree.query(0, 0, 0, 10) == 0); // no x in range
    REQUIRE(tree.query(1, 8, 100, 200) == 0); // no y in range

    // Rectangle capturing a single interior point.
    REQUIRE(tree.query(4, 6, 4, 4) == 1); // only (6,4)

    // Empty ranges (degenerate query bounds).
    REQUIRE(tree.query(5, 3, 0, 10) == 0);
    REQUIRE(tree.query(0, 10, 5, 3) == 0);
}

TEST_CASE("query respects inclusive boundaries", "[layered_range_tree]")
{
    LayeredRangeTree<int> tree{
        {0, 0},
        {1, 1},
        {2, 2},
        {3, 3},
        {4, 4}
    };

    // Exactly matching the min/max bounds should include endpoints.
    REQUIRE(tree.query(0, 4, 0, 4) == 5);
    REQUIRE(tree.query(1, 3, 1, 3) == 3);
    REQUIRE(tree.query(2, 2, 2, 2) == 1);

    // Just missing an endpoint should exclude it.
    REQUIRE(tree.query(1, 4, 1, 4) == 4);
    REQUIRE(tree.query(0, 3, 0, 3) == 4);
    REQUIRE(tree.query(0, 4, 0, 3) == 4);
    REQUIRE(tree.query(0, 4, 1, 4) == 4);
}

TEST_CASE("duplicate x coordinates", "[layered_range_tree]")
{
    // All points share x == 3, sorted by y since operator< on pair
    // compares the second element when first elements are equal.
    LayeredRangeTree<int> tree{
        {3, 1},
        {3, 2},
        {3, 3},
        {3, 4},
        {3, 5}
    };

    REQUIRE(tree.size() == 5);

    REQUIRE(tree.query(3, 3, 1, 5) == 5);
    REQUIRE(tree.query(3, 3, 2, 4) == 3);
    REQUIRE(tree.query(2, 4, 0, 10) == 5);
    REQUIRE(tree.query(4, 10, 0, 10) == 0);
    REQUIRE(tree.query(3, 3, 6, 10) == 0);
}

TEST_CASE("negative coordinates", "[layered_range_tree]")
{
    LayeredRangeTree<int> tree{
        {-10, -5},
        { -3,  2},
        {  0,  0},
        {  4, -8},
        {  7,  6}
    };

    REQUIRE(tree.size() == 5);

    REQUIRE(tree.query(-10, 7, -8, 6) == 5);
    REQUIRE(tree.query(-10, -1, -10, 10) == 2);
    REQUIRE(tree.query(0, 7, -10, -1) == 1); // (4,-8)
    REQUIRE(tree.query(-3, 4, -1, 2) == 2); // (-3,2),(0,0)
}

TEST_CASE("query on empty x or y range returns zero", "[layered_range_tree]")
{
    LayeredRangeTree<int> tree{
        {1, 1},
        {2, 2},
        {3, 3}
    };

    REQUIRE(tree.query(5, 1, 0, 10) == 0); // l1 > r1
    REQUIRE(tree.query(0, 10, 5, 1) == 0); // l2 > r2
    REQUIRE(tree.query(5, 4, 5, 4) == 0); // both inverted
}

TEST_CASE("iterator-range constructor matches initializer-list constructor",
          "[layered_range_tree]")
{
    std::vector<std::pair<int, int>> pts{
        {1, 1},
        {2, 5},
        {3, 2},
        {5, 5},
        {5, 9},
        {9, 0}
    };

    LayeredRangeTree<int> from_iters(pts.begin(), pts.end());

    REQUIRE(from_iters.size() == pts.size());
    REQUIRE(from_iters.query(1, 9, 0, 9) == pts.size());
    REQUIRE(from_iters.query(2, 5, 2, 9) == 4); // (2,5),(3,2),(5,5),(5,9)
}

TEST_CASE("larger dataset stress-tested against brute force",
          "[layered_range_tree]")
{
    std::mt19937 rng(1234);
    std::uniform_int_distribution<int> coord_dist(-50, 50);

    constexpr int num_points = 300;

    std::vector<std::pair<int, int>> pts;
    pts.reserve(num_points);
    for (int i = 0; i < num_points; ++i)
        pts.emplace_back(coord_dist(rng), coord_dist(rng));

    std::sort(pts.begin(), pts.end());

    LayeredRangeTree<int> tree(pts.begin(), pts.end());
    REQUIRE(tree.size() == pts.size());

    std::uniform_int_distribution<int> query_dist(-60, 60);

    constexpr int num_queries = 500;
    for (int q = 0; q < num_queries; ++q) {
        int a = query_dist(rng);
        int b = query_dist(rng);
        int c = query_dist(rng);
        int d = query_dist(rng);

        int l1 = std::min(a, b);
        int r1 = std::max(a, b);
        int l2 = std::min(c, d);
        int r2 = std::max(c, d);

        std::size_t expected = brute_force_query(pts, l1, r1, l2, r2);
        std::size_t actual = tree.query(l1, r1, l2, r2);

        REQUIRE(actual == expected);
    }
}

TEST_CASE("dataset with many duplicate x values stress-tested",
          "[layered_range_tree]")
{
    std::mt19937 rng(4321);
    std::uniform_int_distribution<int> x_dist(
        0, 5); // small range -> many duplicates
    std::uniform_int_distribution<int> y_dist(-20, 20);

    constexpr int num_points = 200;

    std::vector<std::pair<int, int>> pts;
    pts.reserve(num_points);
    for (int i = 0; i < num_points; ++i)
        pts.emplace_back(x_dist(rng), y_dist(rng));

    std::sort(pts.begin(), pts.end());

    LayeredRangeTree<int> tree(pts.begin(), pts.end());
    REQUIRE(tree.size() == pts.size());

    std::uniform_int_distribution<int> query_x(-1, 6);
    std::uniform_int_distribution<int> query_y(-25, 25);

    constexpr int num_queries = 300;
    for (int q = 0; q < num_queries; ++q) {
        int l1 = query_x(rng);
        int r1 = query_x(rng);
        if (l1 > r1)
            std::swap(l1, r1);

        int l2 = query_y(rng);
        int r2 = query_y(rng);
        if (l2 > r2)
            std::swap(l2, r2);

        std::size_t expected = brute_force_query(pts, l1, r1, l2, r2);
        std::size_t actual = tree.query(l1, r1, l2, r2);

        REQUIRE(actual == expected);
    }
}

TEST_CASE(
    "full x range with restricted y range uses top-level fractional cascade",
    "[layered_range_tree]")
{
    // Covers the early-return branch in query() where l1 <= min_x and r1 >=
    // max_x.
    LayeredRangeTree<int> tree{
        {1, 10},
        {2, 20},
        {3,  5},
        {4, 15},
        {5, 25},
        {6,  0}
    };

    REQUIRE(tree.query(1, 6, -100, 100) == 6);
    REQUIRE(tree.query(-100, 100, -100, 100) == 6);
    REQUIRE(tree.query(1, 6, 10, 20) == 3); // (1,10),(2,20),(4,15)
    REQUIRE(tree.query(1, 6, 0, 0) == 1); // (6,0)
    REQUIRE(tree.query(1, 6, 26, 30) == 0);
}

TEST_CASE("double coordinate type works", "[layered_range_tree]")
{
    LayeredRangeTree<double> tree{
        {0.5, 1.5},
        {1.5, 0.5},
        {2.5, 3.5},
        {3.5, 2.5}
    };

    REQUIRE(tree.size() == 4);
    REQUIRE(tree.query(0.0, 4.0, 0.0, 4.0) == 4);
    REQUIRE(tree.query(0.5, 1.5, 0.0, 2.0) == 2);
    REQUIRE(tree.query(2.5, 3.5, 3.0, 4.0) == 1); // (2.5, 3.5)
    REQUIRE(tree.query(0.6, 1.4, 0.0, 2.0) == 0); // excludes both endpoints
}
