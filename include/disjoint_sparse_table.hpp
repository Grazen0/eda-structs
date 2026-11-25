#pragma once
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

template<typename T, typename Op, T identity>
    requires std::is_trivially_copyable_v<T>
class DisjointSparseTable {
private:
    std::vector<T> m_data;
    std::vector<std::vector<T>> m_sum;
    Op m_op{};

public:
    explicit DisjointSparseTable(std::span<const T> data)
        : m_data{data.begin(), data.end()}
    {
        std::size_t pow2 = 1;
        std::size_t cnt = 0;
        while (pow2 < data.size()) {
            pow2 *= 2;
            ++cnt;
        }

        auto get = [&](std::size_t i) {
            if (i >= data.size())
                return identity;
            return data[i];
        };

        m_sum.resize(cnt, std::vector<T>(pow2));

        for (std::size_t lvl = 0; lvl < m_sum.size(); ++lvl) {
            for (std::size_t blk = 0; blk < (1UZ << lvl); ++blk) {
                std::size_t start = blk << (m_sum.size() - lvl);
                std::size_t end = (blk + 1) << (m_sum.size() - lvl);
                std::size_t mid = std::midpoint(start, end);

                m_sum[lvl][mid] = get(mid);
                for (std::size_t i = mid + 1; i < end; ++i)
                    m_sum[lvl][i] = m_op(m_sum[lvl][i - 1], get(i));

                m_sum[lvl][mid - 1] = get(mid - 1);
                for (std::size_t i = mid - 1; i > start; --i)
                    m_sum[lvl][i - 1] = m_op(get(i - 1), m_sum[lvl][i]);
            }
        }
    }

    [[nodiscard]] T query(std::size_t l, std::size_t r) const
    {
        if (l > r)
            throw std::invalid_argument("invalid query range");

        if (r >= m_data.size())
            throw std::out_of_range("range out of range");

        if (l == r)
            return m_data[l];

        std::size_t lvl = __builtin_clzll(l ^ r) - (SIZE_WIDTH - m_sum.size());
        return m_op(m_sum[lvl][l], m_sum[lvl][r]);
    }
};
