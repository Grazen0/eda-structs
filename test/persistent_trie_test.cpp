#include "persistent_trie.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

TEST_CASE("new trie contains nothing", "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    REQUIRE_FALSE(trie.contains(v0, "a"));
    REQUIRE_FALSE(trie.contains(v0, "hello"));
    REQUIRE_FALSE(trie.contains(v0, "z"));
}

TEST_CASE("insert adds a single word", "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();
    auto v1 = trie.insert(v0, "cat");

    REQUIRE(trie.contains(v1, "cat"));
    REQUIRE_FALSE(trie.contains(v1, "dog"));
    REQUIRE_FALSE(trie.contains(v1, "ca"));
    REQUIRE_FALSE(trie.contains(v1, "cats"));
}

TEST_CASE("insert returns a new version, leaving the original untouched",
          "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();
    auto v1 = trie.insert(v0, "cat");

    REQUIRE_FALSE(trie.contains(v0, "cat"));
    REQUIRE(trie.contains(v1, "cat"));
}

TEST_CASE("multiple words can be inserted into the same version chain",
          "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "cat");
    auto v2 = trie.insert(v1, "car");
    auto v3 = trie.insert(v2, "dog");

    REQUIRE(trie.contains(v3, "cat"));
    REQUIRE(trie.contains(v3, "car"));
    REQUIRE(trie.contains(v3, "dog"));
    REQUIRE_FALSE(trie.contains(v3, "ca"));
    REQUIRE_FALSE(trie.contains(v3, "do"));
    REQUIRE_FALSE(trie.contains(v3, "doge"));
}

TEST_CASE("inserting the same word twice keeps it present", "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "cat");
    auto v2 = trie.insert(v1, "cat");

    REQUIRE(trie.contains(v1, "cat"));
    REQUIRE(trie.contains(v2, "cat"));
}

TEST_CASE("words sharing a common prefix are distinguished",
          "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "car");
    auto v2 = trie.insert(v1, "cart");
    auto v3 = trie.insert(v2, "card");

    REQUIRE(trie.contains(v3, "car"));
    REQUIRE(trie.contains(v3, "cart"));
    REQUIRE(trie.contains(v3, "card"));
    REQUIRE_FALSE(trie.contains(v3, "ca"));
    REQUIRE_FALSE(trie.contains(v3, "carts"));
    REQUIRE_FALSE(trie.contains(v3, "carti"));
}

TEST_CASE("each version reflects only the words inserted up to that point",
          "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "a");
    auto v2 = trie.insert(v1, "b");
    auto v3 = trie.insert(v2, "c");

    REQUIRE_FALSE(trie.contains(v0, "a"));
    REQUIRE_FALSE(trie.contains(v0, "b"));
    REQUIRE_FALSE(trie.contains(v0, "c"));

    REQUIRE(trie.contains(v1, "a"));
    REQUIRE_FALSE(trie.contains(v1, "b"));
    REQUIRE_FALSE(trie.contains(v1, "c"));

    REQUIRE(trie.contains(v2, "a"));
    REQUIRE(trie.contains(v2, "b"));
    REQUIRE_FALSE(trie.contains(v2, "c"));

    REQUIRE(trie.contains(v3, "a"));
    REQUIRE(trie.contains(v3, "b"));
    REQUIRE(trie.contains(v3, "c"));
}

TEST_CASE("branching from an earlier version keeps both branches independent",
          "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "cat");

    auto v2a = trie.insert(v1, "dog");
    auto v2b = trie.insert(v1, "fish");

    REQUIRE(trie.contains(v2a, "cat"));
    REQUIRE(trie.contains(v2a, "dog"));
    REQUIRE_FALSE(trie.contains(v2a, "fish"));

    REQUIRE(trie.contains(v2b, "cat"));
    REQUIRE(trie.contains(v2b, "fish"));
    REQUIRE_FALSE(trie.contains(v2b, "dog"));

    REQUIRE(trie.contains(v1, "cat"));
    REQUIRE_FALSE(trie.contains(v1, "dog"));
    REQUIRE_FALSE(trie.contains(v1, "fish"));
}

TEST_CASE("single character words", "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "a");
    auto v2 = trie.insert(v1, "z");

    REQUIRE(trie.contains(v2, "a"));
    REQUIRE(trie.contains(v2, "z"));
    REQUIRE_FALSE(trie.contains(v2, "m"));
}

TEST_CASE("many words inserted sequentially", "[persistent_trie]")
{
    PersistentTrie<> trie;
    auto v = trie.init();

    std::vector<std::string> words{
        "apple",   "app", "application", "banana", "band",
        "bandana", "can", "candy",       "candle", "dog",
    };

    std::vector<typename PersistentTrie<>::Version> versions;
    versions.push_back(v);

    for (const auto& word : words) {
        v = trie.insert(v, word);
        versions.push_back(v);
    }

    for (const auto& word : words)
        REQUIRE(trie.contains(v, word));

    REQUIRE_FALSE(trie.contains(versions.front(), "apple"));

    for (std::size_t i = 0; i < words.size(); ++i) {
        REQUIRE(trie.contains(versions[i + 1], words[i]));

        for (std::size_t j = i + 1; j < words.size(); ++j)
            REQUIRE_FALSE(trie.contains(versions[i + 1], words[j]));
    }
}

TEST_CASE("custom base and range size supports a different alphabet",
          "[persistent_trie]")
{
    PersistentTrie<'0', 10> trie;
    auto v0 = trie.init();

    auto v1 = trie.insert(v0, "123");
    auto v2 = trie.insert(v1, "007");
    auto v3 = trie.insert(v2, "1234");

    REQUIRE(trie.contains(v3, "123"));
    REQUIRE(trie.contains(v3, "007"));
    REQUIRE(trie.contains(v3, "1234"));
    REQUIRE_FALSE(trie.contains(v3, "12"));
    REQUIRE_FALSE(trie.contains(v3, "00"));
    REQUIRE_FALSE(trie.contains(v3, "12345"));
}
