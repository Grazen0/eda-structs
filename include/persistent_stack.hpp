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
        struct Value {
            T top;
            std::size_t next;
        };

        std::optional<Value> value = std::nullopt;

        Node() = default;

        Node(T top, std::size_t next)
            : value{
                  Value{std::move(top), next}
        }
        {
        }
    };

    std::vector<Node> roots{Node{}};

    template<typename... Args>
    [[nodiscard]] constexpr Version emplace_version(Args&&... args)
    {
        std::size_t p = roots.size();
        roots.emplace_back(std::forward<Args>(args)...);
        return Version{p};
    }

    [[nodiscard]] constexpr Node& get_version_root(Version v)
    {
        if (v.idx >= roots.size())
            throw std::out_of_range("stack version out of range");

        return roots[v.idx];
    }

public:
    [[nodiscard]] constexpr Version init()
    {
        return Version{0};
    }

    [[nodiscard]] constexpr Version push(Version v, T value)
    {
        return emplace_version(std::move(value), v.idx);
    }

    [[nodiscard]] constexpr std::optional<T> peek(Version v)
    {
        auto& node = get_version_root(v);
        if (!node.value)
            return std::nullopt;

        return node.value->top;
    }

    [[nodiscard]] constexpr Version pop(Version v)
    {
        auto& node = get_version_root(v);
        if (!node.value)
            throw std::out_of_range("cannot pop from empty stack");

        return Version{node.value->next};
    };
};

#endif
