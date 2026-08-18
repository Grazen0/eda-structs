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
public:
    constexpr void insert(T value)
    {
        emplace(std::move(value));
    }

    template<typename... Args>
    void emplace(Args&&... args)
    {
        data.emplace_back(std::forward<Args>(args)...);
        bubble_up(data.size() - 1);
    }

    std::optional<T> pop()
    {
        if (data.empty())
            return std::nullopt;

        std::swap(data.front(), data.back());

        T retval = std::move(data.back());
        data.pop_back();

        if (!data.empty())
            bubble_down(0);

        return retval;
    }

    [[nodiscard]] constexpr std::optional<std::reference_wrapper<const T>>
    peek() const
    {
        if (data.empty())
            return std::nullopt;

        return data[0];
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return data.empty();
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return data.size();
    }

private:
    std::vector<T> data;
    Compare cmp{};

    void bubble_up(std::size_t i)
    {
        assert(i < data.size());

        if (i == 0)
            return;

        std::size_t p = (i - 1) / 2;

        if (cmp(data[p], data[i])) {
            std::swap(data[p], data[i]);
            bubble_up(p);
        }
    }

    void bubble_down(std::size_t i)
    {
        assert(i < data.size());

        std::size_t l = (2 * i) + 1;
        std::size_t r = (2 * i) + 2;

        std::size_t b = i;

        if (l < data.size() && cmp(data[b], data[l]))
            b = l;

        if (r < data.size() && cmp(data[b], data[r]))
            b = r;

        if (i != b) {
            std::swap(data[i], data[b]);
            bubble_down(b);
        }
    }
};

#endif
