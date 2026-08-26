#ifndef EDA_STRUCTS_PERSISTENT_STACK_HPP
#define EDA_STRUCTS_PERSISTENT_STACK_HPP

#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

template<typename T>
class PersistentStack {
public:
    class Version {
    private:
        explicit Version(std::size_t idx)
            : idx{idx}
        {
        }

        std::size_t idx;

        friend class PersistentStack;
    };

private:
    struct Node {
        std::optional<T> top = std::nullopt;
        std::optional<std::size_t> next = std::nullopt;
    };

    std::vector<Node> roots{Node{}};

    [[nodiscard]] Version make_version(Node node)
    {
        std::size_t p = roots.size();
        roots.emplace_back(std::move(node));
        return Version{p};
    }

public:
    [[nodiscard]] static Version init()
    {
        return Version{0};
    }

    [[nodiscard]] Version push(Version v, T value)
    {
        Node node{
            .top = std::move(value),
            .next = v.idx,
        };

        return make_version(std::move(node));
    }

    [[nodiscard]] std::optional<T> peek(Version v)
    {
        if (v.idx >= roots.size())
            throw std::out_of_range("stack version out of range");

        return roots[v.idx].top;
    }

    [[nodiscard]] Version pop(Version v)
    {
        if (v.idx >= roots.size())
            throw std::out_of_range("stack version out of range");

        auto next = roots[v.idx].next;
        if (!next)
            throw std::out_of_range("cannot pop from empty stack");

        return Version{*next};
    };
};

#endif
