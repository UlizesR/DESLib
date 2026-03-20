/*  history.hpp  –  DES namespace
 *  Ring-buffer history for delay-differential integrators.
 *
 *  Classes:
 *    DES::RingBuffer<T>             – power-of-two circular buffer
 *    DES::CompoundRingBuffer<V,T>   – per-variable delay windowing
 *    DES::History<V,T>              – Hermite-interpolated DDE history
 *
 *  Requires C++17.
 */
#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>

namespace DES {

template <class T>
class RingBuffer {
  private:
    std::size_t m_cap;
    std::size_t m_mask;
    std::vector<T> m_data;
    std::size_t m_head;
    std::size_t m_tail;

    [[nodiscard]] std::size_t wrap(std::size_t i) const noexcept
    {
        return i & m_mask;
    }

    void expand()
    {
        const std::size_t sz = size();
        std::vector<T> tmp(m_cap * 2);
        for (std::size_t i = 0; i < sz; ++i)
        {
            tmp[i] = (*this)[i];
        }
        m_head = 0;
        m_tail = sz;
        m_cap *= 2;
        m_mask = m_cap - 1;
        m_data = std::move(tmp);
    }

  public:
    RingBuffer()
        : m_cap(4)
        , m_mask(3)
        , m_data(4)
        , m_head(3)
        , m_tail(0)
    {}

    T &operator[](std::size_t i) noexcept
    {
        return m_data[wrap(i + m_head)];
    }

    const T &operator[](std::size_t i) const noexcept
    {
        return m_data[wrap(i + m_head)];
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return wrap(m_tail - m_head);
    }

    void advance() noexcept
    {
        m_head = wrap(m_head - 1);
        m_tail = wrap(m_tail - 1);
    }

    void extend()
    {
        if (m_head == wrap(m_tail + 1))
        {
            expand();
        }
        m_head = wrap(m_head - 1);
    }

    void push_front(const T &entry)
    {
        extend();
        m_data[m_head] = entry;
    }
};

template <class V, class T = double>
class CompoundRingBuffer {
  public:
    RingBuffer<T> timestamp;
    RingBuffer<T> h;

  private:
    std::vector<RingBuffer<V>> m_vars;
    std::vector<T> m_max_delays;

  public:
    CompoundRingBuffer() = default;

    CompoundRingBuffer(std::size_t n, T t0, T h0, const std::vector<T> &max_delays, const std::vector<V> &init_vals)
        : m_vars(n)
        , m_max_delays(max_delays)
    {
        assert(max_delays.size() >= n && init_vals.size() >= n);

        timestamp[0] = t0 - T{1};
        timestamp.push_front(t0);
        h[0] = h0;
        h.push_front(h0);

        for (std::size_t k = 0; k < n; ++k)
        {
            m_vars[k][0] = init_vals[k];
            m_vars[k].push_front(init_vals[k]);
        }
    }

    void update(T t, T h_new, const std::vector<V> &vals)
    {
        assert(vals.size() >= m_vars.size());
        const T oldest_t = timestamp[timestamp.size() - 1];
        bool any_ext = false;

        for (std::size_t k = 0; k < m_vars.size(); ++k)
        {
            if (oldest_t < t - m_max_delays[k])
            {
                m_vars[k].advance();
            }
            else
            {
                m_vars[k].extend();
                any_ext = true;
            }
            m_vars[k][0] = vals[k];
        }

        if (any_ext)
        {
            timestamp.extend();
            h.extend();
        }
        else
        {
            timestamp.advance();
            h.advance();
        }

        timestamp[0] = t;
        h[0] = h_new;
    }

    [[nodiscard]] std::size_t bisect(T target) const
    {
        std::size_t lo = 0;
        std::size_t hi = timestamp.size() - 1;

        while (lo < hi)
        {
            const std::size_t mid = (lo + hi) / 2;
            if (timestamp[mid] < target)
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }

        while (lo > 0 && timestamp[lo] == timestamp[lo - 1])
        {
            --lo;
        }
        return lo;
    }

    [[nodiscard]] const RingBuffer<V> &operator[](std::size_t vi) const
    {
        return m_vars[vi];
    }

    [[nodiscard]] RingBuffer<V> &operator[](std::size_t vi)
    {
        return m_vars[vi];
    }

    [[nodiscard]] const std::vector<T> &max_delays() const noexcept
    {
        return m_max_delays;
    }

    [[nodiscard]] T max_delay(std::size_t vi) const
    {
        return m_max_delays.at(vi);
    }

    [[nodiscard]] T min_delay() const
    {
        T out = std::numeric_limits<T>::infinity();
        for (const T d : m_max_delays)
        {
            if (d < out)
            {
                out = d;
            }
        }
        return out;
    }
};

template <class V, class T = double>
class History {
  private:
    T m_t0{};
    std::size_t m_n{};
    std::vector<std::function<V(T)>> m_prehistory;
    mutable std::vector<std::size_t> m_cache;

  public:
    CompoundRingBuffer<std::array<V, 2>, T> _history;

    History() = default;

    History(std::size_t n, T t0, T h0, const std::vector<V> &max_delays, const std::vector<V> &init_conds, const std::vector<std::function<V(T)>> &prehistory)
        : m_t0(t0)
        , m_n(n)
        , m_prehistory(prehistory)
        , m_cache(n, std::size_t{0})
    {
        assert(init_conds.size() >= n && prehistory.size() >= n && max_delays.size() >= n);
        std::vector<std::array<V, 2>> pairs(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            pairs[i] = {init_conds[i], V{}};
        }
        _history = CompoundRingBuffer<std::array<V, 2>, T>(n, t0, h0, max_delays, pairs);
    }

    void save(T time, const std::vector<V> &y1, const std::vector<V> &k1)
    {
        assert(y1.size() >= m_n && k1.size() >= m_n);
        std::vector<std::array<V, 2>> entries(m_n);
        for (std::size_t i = 0; i < m_n; ++i)
        {
            entries[i] = {y1[i], k1[i]};
        }
        _history.update(time, T{}, entries);
    }

    void set_initial_derivatives(const std::vector<V> &dydt0)
    {
        assert(dydt0.size() >= m_n);
        for (std::size_t i = 0; i < m_n; ++i)
        {
            for (std::size_t j = 0; j < _history[i].size(); ++j)
            {
                _history[i][j][1] = dydt0[i];
            }
        }
    }

    [[nodiscard]] T t0() const noexcept
    {
        return m_t0;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_n;
    }

    [[nodiscard]] T min_delay() const
    {
        return _history.min_delay();
    }

    [[nodiscard]] const std::vector<T> &max_delays() const noexcept
    {
        return _history.max_delays();
    }

    [[nodiscard]] V at_time(T t, std::size_t var) const
    {
        if (t < m_t0)
        {
            return m_prehistory[var](t);
        }

        const std::size_t ci = m_cache[var];
        if (ci > 0 && _history.timestamp[ci] <= t && _history.timestamp[ci - 1] > t)
        {
            return interp(t, ci, var);
        }

        const std::size_t idx = _history.bisect(t);
        m_cache[var] = idx;
        if (_history.timestamp[idx] == t)
        {
            return _history[var][idx][0];
        }

        return interp(t, idx, var);
    }

  private:
    [[nodiscard]] V interp(T target, std::size_t idx, std::size_t var) const
    {
        const T t0 = _history.timestamp[idx];
        const T t1 = _history.timestamp[idx - 1];
        const V y0 = _history[var][idx][0];
        const V y1 = _history[var][idx - 1][0];
        const V m0 = _history[var][idx][1];
        const V m1 = _history[var][idx - 1][1];
        const T dt = t1 - t0;
        const T u = T{1} - (t1 - target) / dt;
        const T u2 = u * u;
        const T u3 = u2 * u;

        return (T{2} * u3 - T{3} * u2 + T{1}) * y0 + (-T{2} * u3 + T{3} * u2) * y1 + (u3 - T{2} * u2 + u) * (m0 * dt) + (u3 - u2) * (m1 * dt);
    }
};

}  // namespace DES
