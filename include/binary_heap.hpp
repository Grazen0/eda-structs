#ifndef EDA_STRUCTS_BINARY_HEAP_HPP
#define EDA_STRUCTS_BINARY_HEAP_HPP

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

template<typename T, typename Compare = std::less<T>>
class BinaryHeap {
private:
    std::vector<T> m_data;
    Compare m_cmp{};

    void bubble_up(std::size_t i)
    {
        assert(i < m_data.size());

        if (i == 0)
            return;

        std::size_t p = (i - 1) / 2;

        if (m_cmp(m_data[i], m_data[p])) {
            std::swap(m_data[i], m_data[p]);
            bubble_up(p);
        }
    }

    void bubble_down(std::size_t i)
    {
        assert(i < m_data.size());

        std::size_t l = (2 * i) + 1;
        std::size_t r = (2 * i) + 2;

        std::size_t b = i;

        if (l < m_data.size() && m_cmp(m_data[l], m_data[b]))
            b = l;

        if (r < m_data.size() && m_cmp(m_data[r], m_data[b]))
            b = r;

        if (i != b) {
            std::swap(m_data[i], m_data[b]);
            bubble_down(b);
        }
    }

public:
    constexpr void insert(T value)
    {
        emplace(std::move(value));
    }

    template<typename... Args>
    void emplace(Args&&... args)
    {
        m_data.emplace_back(std::forward<Args>(args)...);
        bubble_up(m_data.size() - 1);
    }

    std::optional<T> pop()
    {
        if (m_data.empty())
            return std::nullopt;

        T retval = std::exchange(m_data.front(), m_data.back());
        m_data.pop_back();

        if (!m_data.empty())
            bubble_down(0);

        return retval;
    }

    [[nodiscard]] constexpr std::optional<std::reference_wrapper<const T>>
    peek() const
    {
        if (m_data.empty())
            return std::nullopt;

        return m_data[0];
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_data.empty();
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return m_data.size();
    }
};

#endif
