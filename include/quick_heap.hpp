#ifndef EDA_STRUCTS_QUICK_HEAP
#define EDA_STRUCTS_QUICK_HEAP

#include <cassert>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <random>
#include <utility>
#include <vector>

template<typename T, typename Compare = std::less<T>>
class QuickHeap {
private:
    std::deque<T> m_heap;
    std::vector<std::size_t> m_pivots{m_heap.size()};
    std::size_t pivot_offset = 0;
    Compare m_cmp;
    std::mt19937_64 m_rnd;

    [[nodiscard]] std::size_t partition(std::size_t r, std::size_t pivot_idx)
    {
        assert(r < m_heap.size());

        std::swap(m_heap.at(pivot_idx), m_heap.at(r));

        std::size_t q = 0;
        const auto& pivot = m_heap.at(r);

        for (std::size_t i = 0; i <= r; ++i) {
            if (!m_cmp(pivot, m_heap.at(i)))
                std::swap(m_heap.at(i), m_heap.at(q++));
        }

        return q - 1;
    }

    void iqs()
    {
        assert(!m_heap.empty());

        while (0 != m_pivots.back() - pivot_offset) {
            std::size_t r = m_pivots.back() - pivot_offset - 1;
            std::uniform_int_distribution<std::size_t> dist{0, r};
            std::size_t pivot_idx = dist(m_rnd);
            std::size_t q = partition(r, pivot_idx);
            m_pivots.push_back(q + pivot_offset);
        }

        m_pivots.pop_back();
    }

public:
    explicit QuickHeap(Compare cmp = {})
        : m_cmp{std::move(cmp)},
          m_rnd(std::chrono::steady_clock::now().time_since_epoch().count())
    {
    }

    QuickHeap(std::initializer_list<T> data, Compare cmp = {})
        : m_heap{data},
          m_cmp{std::move(cmp)},
          m_rnd(std::chrono::steady_clock::now().time_since_epoch().count())
    {
    }

    template<typename Iter>
    QuickHeap(Iter begin, Iter end, Compare cmp = {})
        : m_heap{std::move(begin), std::move(end)},
          m_cmp{std::move(cmp)},
          m_rnd(std::chrono::steady_clock::now().time_since_epoch().count())
    {
    }

    constexpr void insert(T value)
    {
        emplace(std::move(value));
    }

    template<typename... Args>
    void emplace(Args&&... args)
    {
        m_heap.emplace_back();
        std::size_t i = m_heap.size() - 1;

        T value{std::forward<Args>(args)...};

        ++m_pivots[0];
        std::size_t p = 1;

        while (p < m_pivots.size() &&
               !m_cmp(m_heap.at(m_pivots[p] - pivot_offset), value)) {
            m_heap.at(i) =
                std::exchange(m_heap.at(m_pivots[p] - pivot_offset + 1),
                              std::move(m_heap.at(m_pivots[p] - pivot_offset)));
            i = m_pivots[p++]++ - pivot_offset;
        }

        m_heap.at(i) = std::move(value);
    }

    std::optional<T> pop()
    {
        if (m_heap.empty())
            return std::nullopt;

        iqs();
        T retval = std::move(m_heap.front());
        m_heap.pop_front();
        ++pivot_offset;

        return retval;
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const T>> peek()
    {
        if (m_heap.empty())
            return std::nullopt;

        iqs();
        return m_heap.front();
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        return m_heap.size();
    }

    [[nodiscard]] constexpr bool empty() const
    {
        return m_heap.empty();
    }
};

#endif
