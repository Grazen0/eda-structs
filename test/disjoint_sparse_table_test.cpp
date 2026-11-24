#include "disjoint_sparse_table.hpp"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <cstddef>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <span>
#include <vector>

namespace
{

    struct MinOp {
        int operator()(int a, int b) const
        {
            return std::min(a, b);
        }
    };

    struct MaxOp {
        int operator()(int a, int b) const
        {
            return std::max(a, b);
        }
    };

    struct GcdOp {
        int operator()(int a, int b) const
        {
            return std::gcd(a, b);
        }
    };

    struct XorOp {
        unsigned operator()(unsigned a, unsigned b) const
        {
            return a ^ b;
        }
    };

    // Non-commutative, associative operation: 2x2 matrix multiplication (mod
    // p). Aggregate with public members, so it is usable as a non-type template
    // parameter.
    struct Mat2 {
        long long a, b, c, d;
        bool operator==(const Mat2&) const = default;
    };

    std::ostream& operator<<(std::ostream& os, const Mat2& m)
    {
        return os << "[[" << m.a << ", " << m.b << "], [" << m.c << ", " << m.d
                  << "]]";
    }

    constexpr long long MOD = 1'000'003;

    struct MatMul {
        Mat2 operator()(const Mat2& x, const Mat2& y) const
        {
            return {
                (x.a * y.a + x.b * y.c) % MOD, (x.a * y.b + x.b * y.d) % MOD,
                (x.c * y.a + x.d * y.c) % MOD, (x.c * y.b + x.d * y.d) % MOD};
        }
    };

    constexpr Mat2 mat_identity{1, 0, 0, 1};

    using SumTable = DisjointSparseTable<int, std::plus<int>, 0>;
    using MinTable =
        DisjointSparseTable<int, MinOp, std::numeric_limits<int>::max()>;
    using MaxTable =
        DisjointSparseTable<int, MaxOp, std::numeric_limits<int>::min()>;
    using GcdTable = DisjointSparseTable<int, GcdOp, 0>;
    using XorTable = DisjointSparseTable<unsigned, XorOp, 0u>;
    using MatTable = DisjointSparseTable<Mat2, MatMul, mat_identity>;

    // Reference implementation: fold [l, r] (inclusive) left to right.
    template<typename T, typename Op, T identity>
    T naive(const std::vector<T>& v, std::size_t l, std::size_t r)
    {
        T acc = identity;
        for (std::size_t i = l; i <= r; ++i)
            acc = Op{}(acc, v[i]);
        return acc;
    }

    std::vector<int> make_data(std::size_t n, int lo = -100, int hi = 100,
                               unsigned seed = 12345)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(lo, hi);
        std::vector<int> v(n);
        for (auto& x : v)
            x = dist(rng);
        return v;
    }

} // namespace

TEST_CASE("single element table", "[disjoint_sparse_table]")
{
    std::vector<int> data{42};
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 0) == 42);
}

TEST_CASE("two element table", "[disjoint_sparse_table]")
{
    std::vector<int> data{3, 4};
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 0) == 3);
    REQUIRE(table.query(1, 1) == 4);
    REQUIRE(table.query(0, 1) == 7);
}

TEST_CASE("sum queries on a small known array", "[disjoint_sparse_table]")
{
    std::vector<int> data{1, 2, 3, 4, 5};
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 4) == 15);
    REQUIRE(table.query(0, 0) == 1);
    REQUIRE(table.query(4, 4) == 5);
    REQUIRE(table.query(1, 3) == 9);
    REQUIRE(table.query(2, 4) == 12);
    REQUIRE(table.query(0, 2) == 6);
}

TEST_CASE("single point queries return the element itself",
          "[disjoint_sparse_table]")
{
    auto data = make_data(37);
    SumTable table{std::span<int>(data)};

    for (std::size_t i = 0; i < data.size(); ++i)
        REQUIRE(table.query(i, i) == data[i]);
}

TEST_CASE("full range query", "[disjoint_sparse_table]")
{
    auto data = make_data(50);
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, data.size() - 1) ==
            std::accumulate(data.begin(), data.end(), 0));
}

TEST_CASE("all ranges match naive result for many sizes",
          "[disjoint_sparse_table]")
{
    // Mix of powers of two, one below, and one above, plus odd sizes.
    const std::size_t n = GENERATE(1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32,
                                   33, 63, 64, 65, 100);

    auto data = make_data(n, -1000, 1000, static_cast<unsigned>(n));
    SumTable table{std::span<int>(data)};

    for (std::size_t l = 0; l < n; ++l)
        for (std::size_t r = l; r < n; ++r)
            REQUIRE(table.query(l, r) ==
                    (naive<int, std::plus<int>, 0>(data, l, r)));
}

TEST_CASE("queries crossing block midpoints", "[disjoint_sparse_table]")
{
    // Size 16: levels split at 8, then 4/12, then 2/6/10/14, ...
    std::vector<int> data(16);
    std::iota(data.begin(), data.end(), 1);
    SumTable table{std::span<int>(data)};

    // Adjacent pairs straddling every boundary.
    for (std::size_t i = 0; i + 1 < data.size(); ++i)
        REQUIRE(table.query(i, i + 1) == data[i] + data[i + 1]);

    // Ranges that straddle the top-level midpoint.
    REQUIRE(table.query(7, 8) == 8 + 9);
    REQUIRE(table.query(0, 15) == 136);
    REQUIRE(table.query(3, 12) == (naive<int, std::plus<int>, 0>(data, 3, 12)));
}

TEST_CASE("min queries", "[disjoint_sparse_table]")
{
    std::vector<int> data{5, 2, 8, 1, 9, 3, 7};
    MinTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 6) == 1);
    REQUIRE(table.query(0, 2) == 2);
    REQUIRE(table.query(4, 6) == 3);
    REQUIRE(table.query(2, 2) == 8);
    REQUIRE(table.query(3, 4) == 1);
}

TEST_CASE("max queries", "[disjoint_sparse_table]")
{
    std::vector<int> data{5, 2, 8, 1, 9, 3, 7};
    MaxTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 6) == 9);
    REQUIRE(table.query(0, 2) == 8);
    REQUIRE(table.query(5, 6) == 7);
    REQUIRE(table.query(1, 1) == 2);
    REQUIRE(table.query(3, 3) == 1);
}

TEST_CASE("gcd queries", "[disjoint_sparse_table]")
{
    std::vector<int> data{12, 18, 24, 36, 5, 10, 15};
    GcdTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 3) == 6);
    REQUIRE(table.query(0, 6) == 1);
    REQUIRE(table.query(4, 6) == 5);
    REQUIRE(table.query(2, 3) == 12);
    REQUIRE(table.query(1, 1) == 18);
}

TEST_CASE("xor queries", "[disjoint_sparse_table]")
{
    std::vector<unsigned> data{1, 2, 4, 8, 16, 32, 64};
    XorTable table{std::span<unsigned>(data)};

    REQUIRE(table.query(0, 6) == 127u);
    REQUIRE(table.query(0, 2) == 7u);
    REQUIRE(table.query(3, 5) == 56u);
    REQUIRE(table.query(2, 2) == 4u);
}

TEST_CASE("min and max match naive on all ranges", "[disjoint_sparse_table]")
{
    auto data = make_data(45, -50, 50, 7);
    MinTable min_table{std::span<int>(data)};
    MaxTable max_table{std::span<int>(data)};

    for (std::size_t l = 0; l < data.size(); ++l) {
        for (std::size_t r = l; r < data.size(); ++r) {
            REQUIRE(min_table.query(l, r) ==
                    (naive<int, MinOp, std::numeric_limits<int>::max()>(data, l,
                                                                        r)));
            REQUIRE(max_table.query(l, r) ==
                    (naive<int, MaxOp, std::numeric_limits<int>::min()>(data, l,
                                                                        r)));
        }
    }
}

TEST_CASE("negative values", "[disjoint_sparse_table]")
{
    std::vector<int> data{-5, 10, -1, 0, -10, 7};
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 5) == 1);
    REQUIRE(table.query(0, 2) == 4);
    REQUIRE(table.query(2, 4) == -11);
    REQUIRE(table.query(4, 4) == -10);
}

TEST_CASE("identity-valued elements", "[disjoint_sparse_table]")
{
    std::vector<int> data(10, 0);
    SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 9) == 0);
    REQUIRE(table.query(3, 6) == 0);
    REQUIRE(table.query(5, 5) == 0);
}

TEST_CASE("all equal elements", "[disjoint_sparse_table]")
{
    std::vector<int> data(13, 7);
    SumTable sum{std::span<int>(data)};
    MinTable min{std::span<int>(data)};

    REQUIRE(sum.query(0, 12) == 91);
    REQUIRE(sum.query(4, 8) == 35);
    REQUIRE(min.query(0, 12) == 7);
    REQUIRE(min.query(6, 6) == 7);
}

TEST_CASE("non-commutative operation preserves operand order",
          "[disjoint_sparse_table]")
{
    // Distinct, non-symmetric matrices so any reordering changes the product.
    std::vector<Mat2> data;
    for (long long i = 1; i <= 21; ++i)
        data.push_back({i, i + 1, 2 * i % 7 + 1, i * i % 11});

    MatTable table{std::span<Mat2>(data)};

    for (std::size_t l = 0; l < data.size(); ++l)
        for (std::size_t r = l; r < data.size(); ++r)
            REQUIRE(table.query(l, r) ==
                    (naive<Mat2, MatMul, mat_identity>(data, l, r)));
}

TEST_CASE("non-commutative operation on a power of two size",
          "[disjoint_sparse_table]")
{
    std::vector<Mat2> data;
    for (long long i = 1; i <= 16; ++i)
        data.push_back({i % 5 + 1, i % 3, (i * 7) % 4, i % 6 + 1});

    MatTable table{std::span<Mat2>(data)};

    REQUIRE(table.query(0, 15) ==
            (naive<Mat2, MatMul, mat_identity>(data, 0, 15)));
    REQUIRE(table.query(7, 8) ==
            (naive<Mat2, MatMul, mat_identity>(data, 7, 8)));
    REQUIRE(table.query(5, 11) ==
            (naive<Mat2, MatMul, mat_identity>(data, 5, 11)));
}

TEST_CASE("query on a const table", "[disjoint_sparse_table]")
{
    std::vector<int> data{1, 2, 3, 4};
    const SumTable table{std::span<int>(data)};

    REQUIRE(table.query(0, 3) == 10);
    REQUIRE(table.query(1, 2) == 5);
}

TEST_CASE("repeated queries give the same result", "[disjoint_sparse_table]")
{
    std::vector<int> data{4, 8, 15, 16, 23, 42};
    SumTable table{std::span<int>(data)};

    for (int i = 0; i < 3; ++i) {
        REQUIRE(table.query(0, 5) == 108);
        REQUIRE(table.query(2, 4) == 54);
    }
}

TEST_CASE("construction does not modify the input", "[disjoint_sparse_table]")
{
    std::vector<int> data{9, 3, 7, 1, 5, 2, 8};
    const std::vector<int> original = data;

    SumTable table{std::span<int>(data)};
    (void)table.query(0, 6);

    REQUIRE(data == original);
}

TEST_CASE("table is independent of the source after construction",
          "[disjoint_sparse_table]")
{
    std::vector<int> data{1, 2, 3, 4, 5};
    SumTable table{std::span<int>(data)};

    std::fill(data.begin(), data.end(), 100);

    REQUIRE(table.query(0, 4) == 15);
    REQUIRE(table.query(1, 3) == 9);
}

TEST_CASE("constructs from a subspan", "[disjoint_sparse_table]")
{
    std::vector<int> data{100, 1, 2, 3, 4, 100};
    SumTable table{std::span<int>(data).subspan(1, 4)};

    REQUIRE(table.query(0, 3) == 10);
    REQUIRE(table.query(0, 0) == 1);
    REQUIRE(table.query(3, 3) == 4);
}

TEST_CASE("large array with random queries", "[disjoint_sparse_table]")
{
    constexpr std::size_t n = 1000; // deliberately not a power of two
    auto data = make_data(n, -1'000'000, 1'000'000, 99);
    SumTable table{std::span<int>(data)};

    std::mt19937 rng(2024);
    std::uniform_int_distribution<std::size_t> idx(0, n - 1);

    for (int i = 0; i < 5000; ++i) {
        std::size_t a = idx(rng);
        std::size_t b = idx(rng);
        std::size_t l = std::min(a, b);
        std::size_t r = std::max(a, b);

        REQUIRE(table.query(l, r) ==
                (naive<int, std::plus<int>, 0>(data, l, r)));
    }
}

TEST_CASE("large array with min queries on sorted input",
          "[disjoint_sparse_table]")
{
    constexpr std::size_t n = 1024;
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);

    MinTable table{std::span<int>(data)};

    REQUIRE(table.query(0, n - 1) == 0);
    REQUIRE(table.query(500, 900) == 500);
    REQUIRE(table.query(1023, 1023) == 1023);

    std::reverse(data.begin(), data.end());
    MinTable reversed{std::span<int>(data)};

    REQUIRE(reversed.query(0, n - 1) == 0);
    REQUIRE(reversed.query(0, 100) == 923);
}
