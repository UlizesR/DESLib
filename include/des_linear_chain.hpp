#pragma once

#include "des_adaptive.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace DES {

// ---------------------------------------------------------------------------
// DenseSegment<N>
//
// Dense-output polynomial for one accepted step of DoPri54.
// Horner form:
//   y(t₀ + θ·h) = y₀ + h·θ·(q[0] + θ·(q[1] + θ·(q[2] + θ·q[3])))
// ---------------------------------------------------------------------------

template <int N>
struct DenseSegment {
    double t0 = 0.0;
    double h = 0.0;
    bool valid = false;
    Vec<N> y0{};
    std::array<Vec<N>, 4> q{};  // Horner coefficients

    // True when t ∈ [t0, t0+h] (direction-aware, with rounding tolerance)
    [[nodiscard]] bool contains(double t) const noexcept
    {
        if (!valid)
        {
            return false;
        }
        const double t1 = t0 + h;
        const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(t0), std::abs(t1)));
        return (h >= 0.0) ? (t >= t0 - eps && t <= t1 + eps) : (t <= t0 + eps && t >= t1 - eps);
    }

    // Evaluate at θ ∈ [0,1] using Horner's method (8 multiplications)
    [[nodiscard]] Vec<N> eval_theta(double theta) const
    {
        if (!valid)
        {
            throw std::logic_error("DenseSegment: dense output not available");
        }
        theta = std::clamp(theta, 0.0, 1.0);
        return y0 + (h * theta) * (q[0] + theta * (q[1] + theta * (q[2] + theta * q[3])));
    }

    // Evaluate at absolute time t
    [[nodiscard]] Vec<N> eval(double t) const
    {
        if (h == 0.0)
        {
            return y0;
        }
        return eval_theta((t - t0) / h);
    }
};

// ---------------------------------------------------------------------------
// DoPri54<N, HistoryPoints>
//
// Dormand–Prince 5(4) explicit Runge–Kutta solver.
//
// Properties
// ──────────
//   • 5th-order solution, embedded 4th-order error estimate
//   • FSAL: stage k[6] = f(t+h, y_{n+1}) reused as k[0] of the next step
//   • 4th-order dense output (continuous Runge–Kutta, Hairer §II.5)
//   • Breaking-point schedule enforcement with FSAL invalidation
//   • Zero-crossing event detection via AdaptiveDES
// ---------------------------------------------------------------------------

template <int N, int HistoryPoints = 1000>
class DoPri54 : public AdaptiveDES<DoPri54<N, HistoryPoints>, N, HistoryPoints, 7> {
    using Base = AdaptiveDES<DoPri54<N, HistoryPoints>, N, HistoryPoints, 7>;
    using WorkspaceT = Workspace<N, 7>;

    // Butcher tableau (Dormand & Prince 1980)
    struct C {
        static constexpr double c2 = 1.0 / 5.0, c3 = 3.0 / 10.0, c4 = 4.0 / 5.0, c5 = 8.0 / 9.0, c6 = 1.0;

        static constexpr double a21 = 1.0 / 5.0, a31 = 3.0 / 40.0, a32 = 9.0 / 40.0, a41 = 44.0 / 45.0, a42 = -56.0 / 15.0, a43 = 32.0 / 9.0, a51 = 19372.0 / 6561.0, a52 = -25360.0 / 2187.0, a53 = 64448.0 / 6561.0, a54 = -212.0 / 729.0, a61 = 9017.0 / 3168.0, a62 = -355.0 / 33.0,
                                a63 = 46732.0 / 5247.0, a64 = 49.0 / 176.0, a65 = -5103.0 / 18656.0;

        // 5th-order weights
        static constexpr double b1 = 35.0 / 384.0, b3 = 500.0 / 1113.0, b4 = 125.0 / 192.0, b5 = -2187.0 / 6784.0, b6 = 11.0 / 84.0;

        // Error weights (b_5th − b_4th)
        static constexpr double e1 = -71.0 / 57600.0, e3 = 71.0 / 16695.0, e4 = -71.0 / 1920.0, e5 = 17253.0 / 339200.0, e6 = -22.0 / 525.0, e7 = 1.0 / 40.0;

        // Dense-output polynomial coefficients p[stage][degree 1..4]
        static constexpr double p[7][4] = {{1.0, -8048581381.0 / 2820520608.0, 8663915743.0 / 2820520608.0, -12715105075.0 / 11282082432.0},
                                           {0.0, 0.0, 0.0, 0.0},
                                           {0.0, 131558114200.0 / 32700410799.0, -68118460800.0 / 10900136933.0, 87487479700.0 / 32700410799.0},
                                           {0.0, -1754552775.0 / 470086768.0, 14199869525.0 / 1410260304.0, -10690763975.0 / 1880347072.0},
                                           {0.0, 127303824393.0 / 49829197408.0, -318862633887.0 / 49829197408.0, 701980252875.0 / 199316789632.0},
                                           {0.0, -282668133.0 / 205662961.0, 2019193451.0 / 616988883.0, -1453857185.0 / 822651844.0},
                                           {0.0, 40617522.0 / 29380423.0, -110615467.0 / 29380423.0, 69997945.0 / 29380423.0}};
    };

  public:
    DoPri54()
    {
        this->options.controller.kind = ControllerKind::PI;
        this->options.controller.safety = 0.9;
        this->options.controller.min_factor = 0.2;
        this->options.controller.max_factor = 10.0;
    }

    // CRTP capability queries
    [[nodiscard]] static constexpr int method_order()
    {
        return 5;
    }
    [[nodiscard]] static constexpr int adaptive_order()
    {
        return 4;
    }
    [[nodiscard]] static constexpr bool has_fsal()
    {
        return true;
    }

    // ── Dense output ────────────────────────────────────────────────────────

    [[nodiscard]] bool has_dense_output() const noexcept
    {
        return m_last.valid;
    }

    // Most recently committed dense segment (after an accepted step)
    [[nodiscard]] const DenseSegment<N> &last_dense_step() const noexcept
    {
        return m_last;
    }

    [[nodiscard]] int dense_history_size() const noexcept
    {
        return static_cast<int>(m_dense_hist.size());
    }

    [[nodiscard]] const DenseSegment<N> &dense_segment(int i) const
    {
        if (i < 0 || i >= dense_history_size())
        {
            throw std::out_of_range("DoPri54: dense history index out of range");
        }
        return m_dense_hist[static_cast<std::size_t>(i)];
    }

    [[nodiscard]] Vec<N> dense_eval_theta(double theta) const
    {
        return m_last.eval_theta(theta);
    }

    [[nodiscard]] Vec<N> dense_eval(double t) const
    {
        return m_last.eval(t);
    }

    // Search the full dense history for a segment containing t.
    // Binary search over the stored segment list (O(log n)).
    [[nodiscard]] Vec<N> interpolate(double t) const
    {
        const int n = dense_history_size();
        if (n == 0)
        {
            throw std::out_of_range("DoPri54: no dense segments stored");
        }

        // m_dense_hist is ordered chronologically; last seg most likely match
        if (m_dense_hist.back().contains(t))
        {
            return m_dense_hist.back().eval(t);
        }

        // Binary search: each segment's t0 is increasing for forward integration
        int lo = 0, hi = n - 1;
        while (lo <= hi)
        {
            const int mid = (lo + hi) / 2;
            const auto &seg = m_dense_hist[static_cast<std::size_t>(mid)];
            const double dir = (seg.h >= 0.0) ? 1.0 : -1.0;
            const double t1 = seg.t0 + seg.h;
            const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(seg.t0), std::abs(t1)));
            if ((dir > 0.0) ? (t < seg.t0 - eps) : (t > seg.t0 + eps))
            {
                hi = mid - 1;
            }
            else if ((dir > 0.0) ? (t > t1 + eps) : (t < t1 - eps))
            {
                lo = mid + 1;
            }
            else
            {
                return seg.eval(t);  // found
            }
        }
        throw std::out_of_range("DoPri54: interpolation time is outside stored dense history");
    }

    [[nodiscard]] double interpolate_component(double t, int component) const
    {
        return interpolate(t)[component];
    }

    // ── compute_step (called by AdaptiveDES::solve_impl) ────────────────────

    template <typename RhsEval>
    void compute_step(double t, const Vec<N> &y, double h, RhsEval &&rhs, WorkspaceT &ws, SolverStats &stats)
    {
        // Pre-scale h·aij to reduce multiplications in stage sums
        const double ha21 = h * C::a21;
        const double ha31 = h * C::a31, ha32 = h * C::a32;
        const double ha41 = h * C::a41, ha42 = h * C::a42, ha43 = h * C::a43;
        const double ha51 = h * C::a51, ha52 = h * C::a52, ha53 = h * C::a53, ha54 = h * C::a54;
        const double ha61 = h * C::a61, ha62 = h * C::a62, ha63 = h * C::a63, ha64 = h * C::a64, ha65 = h * C::a65;

        rhs(t + C::c2 * h, y + ha21 * ws.k[0], ws.k[1]);
        ++stats.rhs_evals;

        rhs(t + C::c3 * h, y + ha31 * ws.k[0] + ha32 * ws.k[1], ws.k[2]);
        ++stats.rhs_evals;

        rhs(t + C::c4 * h, y + ha41 * ws.k[0] + ha42 * ws.k[1] + ha43 * ws.k[2], ws.k[3]);
        ++stats.rhs_evals;

        rhs(t + C::c5 * h, y + ha51 * ws.k[0] + ha52 * ws.k[1] + ha53 * ws.k[2] + ha54 * ws.k[3], ws.k[4]);
        ++stats.rhs_evals;

        rhs(t + C::c6 * h, y + ha61 * ws.k[0] + ha62 * ws.k[1] + ha63 * ws.k[2] + ha64 * ws.k[3] + ha65 * ws.k[4], ws.k[5]);
        ++stats.rhs_evals;

        // 5th-order solution
        ws.next = y + h * (C::b1 * ws.k[0] + C::b3 * ws.k[2] + C::b4 * ws.k[3] + C::b5 * ws.k[4] + C::b6 * ws.k[5]);

        // FSAL: k[6] = f at new point, reused as k[0] next step
        rhs(t + h, ws.next, ws.k[6]);
        ++stats.rhs_evals;

        // Error estimate (5th − 4th order)
        ws.error = h * (C::e1 * ws.k[0] + C::e3 * ws.k[2] + C::e4 * ws.k[3] + C::e5 * ws.k[4] + C::e6 * ws.k[5] + C::e7 * ws.k[6]);

        ws.fsal = ws.k[6];

        build_dense(t, y, h, ws);
    }

    // ── Lifecycle hooks ──────────────────────────────────────────────────────

    void before_solve()
    {
        m_last = {};
        m_pending = {};
        m_dense_hist.clear();
    }

    // Commit the pending dense segment built during compute_step (called only
    // on accepted steps via AdaptiveDES::after_step)
    void after_step(double /*t*/)
    {
        m_last = m_pending;
        m_dense_hist.push_back(m_pending);
    }

  private:
    DenseSegment<N> m_last{};     // last committed segment
    DenseSegment<N> m_pending{};  // built on trial step
    std::vector<DenseSegment<N>> m_dense_hist{};

    // Build the Horner-form dense polynomial for the current trial step.
    // Stored in m_pending; promoted to m_last only if the step is accepted.
    void build_dense(double t, const Vec<N> &y, double h, const WorkspaceT &ws)
    {
        m_pending.t0 = t;
        m_pending.h = h;
        m_pending.y0 = y;
        m_pending.valid = true;

        m_pending.q[0] = ws.k[0];  // linear term (p[][0] column = k[0])

        m_pending.q[1] = C::p[0][1] * ws.k[0] + C::p[2][1] * ws.k[2] + C::p[3][1] * ws.k[3] + C::p[4][1] * ws.k[4] + C::p[5][1] * ws.k[5] + C::p[6][1] * ws.k[6];

        m_pending.q[2] = C::p[0][2] * ws.k[0] + C::p[2][2] * ws.k[2] + C::p[3][2] * ws.k[3] + C::p[4][2] * ws.k[4] + C::p[5][2] * ws.k[5] + C::p[6][2] * ws.k[6];

        m_pending.q[3] = C::p[0][3] * ws.k[0] + C::p[2][3] * ws.k[2] + C::p[3][3] * ws.k[3] + C::p[4][3] * ws.k[4] + C::p[5][3] * ws.k[5] + C::p[6][3] * ws.k[6];
    }
};

// Backward-compatibility alias
template <int N, int HistoryPoints = 5000>
using DoPri5 = DoPri54<N, HistoryPoints>;

}  // namespace DES