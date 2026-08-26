#include "persistent_stack.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("empty stack has no top", "[persistent_stack]")
{
    PersistentStack<int> st;
    REQUIRE(!st.peek(st.init()));
}

TEST_CASE("push/pop history is preserved across versions", "[persistent_stack]")
{
    PersistentStack<int> st;
    auto v1 = st.push(st.init(), 1);
    auto v2 = st.push(v1, 2);
    auto v3 = st.push(v2, 3);
    auto v4 = st.pop(v3);
    auto v5 = st.pop(v4);
    auto v6 = st.push(v5, 42);

    REQUIRE(!st.peek(st.init()));
    REQUIRE(*st.peek(v1) == 1);
    REQUIRE(*st.peek(v2) == 2);
    REQUIRE(*st.peek(v3) == 3);
    REQUIRE(*st.peek(v4) == 2);
    REQUIRE(*st.peek(v5) == 1);
    REQUIRE(*st.peek(v6) == 42);
}

TEST_CASE("peek does not mutate the version", "[persistent_stack]")
{
    PersistentStack<int> st;
    auto v1 = st.push(st.init(), 10);

    REQUIRE(*st.peek(v1) == 10);
    REQUIRE(*st.peek(v1) == 10);
    REQUIRE(*st.peek(v1) == 10);
}

TEST_CASE("popping an empty stack throws", "[persistent_stack]")
{
    PersistentStack<int> st;
    REQUIRE_THROWS_AS(st.pop(st.init()), std::out_of_range);
}

TEST_CASE("popping down to empty and then popping again throws",
          "[persistent_stack]")
{
    PersistentStack<int> st;
    auto v1 = st.push(st.init(), 1);
    auto v0 = st.pop(v1);

    REQUIRE(!st.peek(v0));
    REQUIRE_THROWS_AS(st.pop(v0), std::out_of_range);
}

TEST_CASE("branching from the same version yields independent futures",
          "[persistent_stack]")
{
    PersistentStack<int> st;
    auto v1 = st.push(st.init(), 1);

    auto branchA = st.push(v1, 100);
    auto branchB = st.push(v1, 200);
    auto branchA2 = st.push(branchA, 101);

    REQUIRE(*st.peek(v1) == 1);
    REQUIRE(*st.peek(branchA) == 100);
    REQUIRE(*st.peek(branchB) == 200);
    REQUIRE(*st.peek(branchA2) == 101);

    // Popping one branch does not affect the other.
    auto poppedA = st.pop(branchA2);
    REQUIRE(*st.peek(poppedA) == 100);
    REQUIRE(*st.peek(branchB) == 200);
    REQUIRE(*st.peek(branchA) == 100);
}

TEST_CASE("works with non-trivial value types", "[persistent_stack]")
{
    PersistentStack<std::string> st;
    auto v1 = st.push(st.init(), std::string{"hello"});
    auto v2 = st.push(v1, std::string{"world"});

    REQUIRE(*st.peek(v1) == "hello");
    REQUIRE(*st.peek(v2) == "world");

    auto v3 = st.pop(v2);
    REQUIRE(*st.peek(v3) == "hello");
}

TEST_CASE("deep linear history stays consistent", "[persistent_stack]")
{
    PersistentStack<int> st;
    std::vector<PersistentStack<int>::Version> versions;
    versions.push_back(st.init());

    constexpr int n = 1000;
    for (int i = 0; i < n; ++i)
        versions.push_back(st.push(versions.back(), i));

    for (int i = 0; i < n; ++i)
        REQUIRE(*st.peek(versions[static_cast<std::size_t>(i) + 1]) == i);

    // Unwind and check every intermediate version is still intact.
    auto cur = versions.back();
    for (int i = n - 1; i >= 0; --i) {
        REQUIRE(*st.peek(cur) == i);
        cur = st.pop(cur);
    }
    REQUIRE(!st.peek(cur));
}

TEST_CASE(
    "using a version from a different stack instance can throw out_of_range",
    "[persistent_stack]")
{
    PersistentStack<int> big;
    auto v1 = big.push(big.init(), 1);
    auto v2 = big.push(v1, 2);
    auto v3 = big.push(v2, 3);
    (void)v1;
    (void)v2;

    PersistentStack<int> small;
    // `small` only has one root version (index 0), so a version with a
    // higher index minted by `big` is out of range for it.
    REQUIRE_THROWS_AS(small.peek(v3), std::out_of_range);
    REQUIRE_THROWS_AS(small.pop(v3), std::out_of_range);
}
