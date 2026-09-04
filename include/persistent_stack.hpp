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
            : m_idx{idx}
        {
        }

        std::size_t m_idx;

        friend class PersistentStack;
    };

private:
    struct Node {
        struct Value {
            T m_top;
            std::size_t m_next;
        };

        std::optional<Value> m_value = std::nullopt;

        Node() = default;

        Node(T top, std::size_t next)
            : m_value{
                  Value{std::move(top), next}
        }
        {
        }
    };

    std::vector<Node> m_roots{Node{}};

    template<typename... Args>
    [[nodiscard]] constexpr Version emplace_version(Args&&... args)
    {
        std::size_t p = m_roots.size();
        m_roots.emplace_back(std::forward<Args>(args)...);
        return Version{p};
    }

    [[nodiscard]] constexpr Node& get_version_root(Version v)
    {
        if (v.m_idx >= m_roots.size())
            throw std::out_of_range("stack version out of range");

        return m_roots[v.m_idx];
    }

public:
    [[nodiscard]] constexpr Version init()
    {
        return Version{0};
    }

    [[nodiscard]] constexpr Version push(Version v, T value)
    {
        return emplace_version(std::move(value), v.m_idx);
    }

    [[nodiscard]] constexpr std::optional<T> peek(Version v)
    {
        auto& node = get_version_root(v);
        if (!node.m_value)
            return std::nullopt;

        return node.m_value->m_top;
    }

    [[nodiscard]] constexpr Version pop(Version v)
    {
        auto& node = get_version_root(v);
        if (!node.m_value)
            throw std::out_of_range("cannot pop from empty stack");

        return Version{node.m_value->m_next};
    };
};

#endif
