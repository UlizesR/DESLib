#pragma once

/*  des_dopri87.hpp  –  DES namespace
 *
 *  Dormand–Prince explicit 8(7) Runge–Kutta pair.
 *
 *  References
 *  ──────────
 *  P. J. Prince, J. R. Dormand.  High order embedded Runge-Kutta formulae.
 *  J. Comput. Appl. Math. 7 (1981) 67–75.
 *
 *  E. Hairer, S. P. Nørsett, G. Wanner.  Solving ODE I.
 *  2nd ed., Springer 1993, §II.5 (DOPRI8 Fortran code, Table 5.4).
 *
 *  Properties
 *  ──────────
 *  • 13 stages, no FSAL (stage 13 cannot be recycled)
 *  • 8th-order solution (b weights using stages 1, 6–13)
 *  • 7th-order embedded estimate (d weights using stages 1, 6–12 only)
 *  • Error = h · (b − d) · k, implemented via precomputed e coefficients
 *  • Dense output: Hermite cubic O(h⁴) from endpoint function evaluations
 *    (one extra RHS eval per step; stored in ws.fsal, has_fsal() = false)
 *  • Recommended tolerances: rtol = atol = 1e-10 … 1e-14
 *
 *  Stage layout in Workspace<N,13>.k[0..12]:
 *    k[0]  = stage 1  = f(t,       y)
 *    k[1]  = stage 2
 *    k[2]  = stage 3
 *    k[3]  = stage 4
 *    k[4]  = stage 5
 *    k[5]  = stage 6   ← first non-zero b/d contribution beyond stage 1
 *    k[6]  = stage 7
 *    k[7]  = stage 8
 *    k[8]  = stage 9
 *    k[9]  = stage 10
 *    k[10] = stage 11
 *    k[11] = stage 12
 *    k[12] = stage 13  ← only in b13; d13 = 0
 */

#include "des_adaptive.hpp"
#include "des_dense_output.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace DES {

template <int N, int HistoryPoints = 500>
class DoPri87 : public AdaptiveDES<DoPri87<N, HistoryPoints>, N, HistoryPoints, 13> {
    using Base = AdaptiveDES<DoPri87<N, HistoryPoints>, N, HistoryPoints, 13>;
    using WorkspaceT = Workspace<N, 13>;

    // ── Butcher tableau (Prince & Dormand 1981, verified against Hairer) ────
    struct C {
        // Abscissae
        static constexpr double c2 = 1.0 / 18.0, c3 = 1.0 / 12.0, c4 = 1.0 / 8.0, c5 = 5.0 / 16.0, c6 = 3.0 / 8.0, c7 = 59.0 / 400.0, c8 = 93.0 / 200.0, c9 = 5490023248.0 / 9719169821.0, c10 = 13.0 / 20.0, c11 = 1201146811.0 / 1299019798.0;
        // c12 = c13 = 1

        // Stage 2 (only a21)
        static constexpr double a21 = 1.0 / 18.0;

        // Stage 3
        static constexpr double a31 = 1.0 / 48.0, a32 = 1.0 / 16.0;

        // Stage 4 (a42 = 0)
        static constexpr double a41 = 1.0 / 32.0, a43 = 3.0 / 32.0;

        // Stage 5 (a52 = 0)
        static constexpr double a51 = 5.0 / 16.0, a53 = -75.0 / 64.0, a54 = 75.0 / 64.0;

        // Stage 6 (a62 = a63 = 0)
        static constexpr double a61 = 3.0 / 80.0, a64 = 3.0 / 16.0, a65 = 3.0 / 20.0;

        // Stage 7 (a72 = a73 = 0)
        static constexpr double a71 = 29443841.0 / 614563906.0, a74 = 77736538.0 / 692538347.0, a75 = -28693883.0 / 1125000000.0, a76 = 23124283.0 / 1800000000.0;

        // Stage 8
        static constexpr double a81 = 16016141.0 / 946692911.0, a84 = 61564180.0 / 158732637.0, a85 = 22789713.0 / 633445777.0, a86 = 545815736.0 / 2771057229.0, a87 = -180193667.0 / 1043307555.0;

        // Stage 9
        static constexpr double a91 = 39632708.0 / 573591083.0, a94 = -433636366.0 / 683701615.0, a95 = -421739975.0 / 2616292301.0, a96 = 100302831.0 / 723423059.0, a97 = 790204164.0 / 839813087.0, a98 = 800635310.0 / 3783071287.0;

        // Stage 10
        static constexpr double a101 = 246121993.0 / 1340847787.0, a104 = -37695042795.0 / 15268766246.0, a105 = -309121744.0 / 1061227803.0, a106 = -12992083.0 / 490766935.0, a107 = 6005943493.0 / 2108947869.0, a108 = 393006217.0 / 1396673457.0, a109 = 123872331.0 / 1001029789.0;

        // Stage 11
        static constexpr double a111 = -1028468189.0 / 846180014.0, a114 = 8478235783.0 / 508512852.0, a115 = 1311729495.0 / 1432422823.0, a116 = -10304129995.0 / 1701304382.0, a117 = -48777925059.0 / 3047939560.0, a118 = 15336726248.0 / 1032824649.0, a119 = -45442868181.0 / 3398467696.0,
                                a1110 = 3065993473.0 / 597172653.0;

        // Stage 12 (c12 = 1)
        static constexpr double a121 = 185892177.0 / 718116043.0, a124 = -3185094517.0 / 667107341.0, a125 = -477755414.0 / 1098053517.0, a126 = -703635378.0 / 230739211.0, a127 = 5731566787.0 / 1027545527.0, a128 = 5232866602.0 / 850066563.0, a129 = -4093664535.0 / 808688257.0,
                                a1210 = 3962137247.0 / 1805957418.0, a1211 = 65686358.0 / 487910083.0;

        // Stage 13 (c13 = 1; a1312 = 0 — does NOT use stage 12)
        static constexpr double a131 = 403863854.0 / 491063109.0, a134 = -5068492393.0 / 434740067.0, a135 = -411421997.0 / 543043805.0, a136 = 652783627.0 / 914296604.0, a137 = 11173962825.0 / 925320556.0, a138 = -13158990841.0 / 6184727034.0, a139 = 3936647629.0 / 1978049680.0,
                                a1310 = -160528059.0 / 685178525.0, a1311 = 248638103.0 / 1413531060.0;

        // 8th-order solution weights b (stages 1, 6–13)
        static constexpr double b1 = 14005451.0 / 335480064.0, b6 = -59238493.0 / 1068277825.0, b7 = 181606767.0 / 758867731.0, b8 = 561292985.0 / 797845732.0, b9 = -1041891430.0 / 1371343529.0, b10 = 760417239.0 / 1151165299.0, b11 = 118820643.0 / 751138087.0, b12 = -528747749.0 / 2220607170.0,
                                b13 = 1.0 / 4.0;

        // 7th-order embedded weights d (stages 1, 6–12; d13 = 0)
        static constexpr double d1 = 13451932.0 / 455176632.0, d6 = -808719846.0 / 976000145.0, d7 = 1757004468.0 / 5645159321.0, d8 = 656045339.0 / 265891186.0, d9 = -3867574721.0 / 1518517206.0, d10 = 465885868.0 / 322736535.0, d11 = 53011238.0 / 667516719.0, d12 = 2.0 / 45.0;

        // Error weights e = b − d (compile-time)
        // e₁₃ = b₁₃ − d₁₃ = b₁₃ (since d₁₃ = 0)
        static constexpr double e1 = b1 - d1, e6 = b6 - d6, e7 = b7 - d7, e8 = b8 - d8, e9 = b9 - d9, e10 = b10 - d10, e11 = b11 - d11, e12 = b12 - d12,
                                e13 = b13;  // d13 = 0
    };

  public:
    DoPri87()
    {
        this->options.controller.kind = ControllerKind::PI;
        this->options.controller.safety = 0.8;
        this->options.controller.min_factor = 0.1;
        this->options.controller.max_factor = 5.0;
    }

    // ── CRTP capability queries ─────────────────────────────────────────────

    [[nodiscard]] static constexpr int method_order()
    {
        return 8;
    }
    [[nodiscard]] static constexpr int adaptive_order()
    {
        return 7;
    }
    [[nodiscard]] static constexpr bool has_fsal()
    {
        return false;
    }

    // ── Dense output (Hermite cubic, O(h⁴)) ─────────────────────────────────

    [[nodiscard]] bool has_dense_output() const noexcept
    {
        return m_last.valid;
    }

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
            throw std::out_of_range("DoPri87: dense history index out of range");
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

    // Binary search over stored segments — O(log n)
    [[nodiscard]] Vec<N> interpolate(double t) const
    {
        const int n = dense_history_size();
        if (n == 0)
        {
            throw std::out_of_range("DoPri87: no dense segments stored");
        }
        if (m_dense_hist.back().contains(t))
        {
            return m_dense_hist.back().eval(t);
        }

        int lo = 0, hi = n - 1;
        while (lo <= hi)
        {
            const int mid = (lo + hi) / 2;
            const auto &seg = m_dense_hist[static_cast<std::size_t>(mid)];
            const double t1 = seg.t0 + seg.h;
            const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(seg.t0), std::abs(t1)));
            const double dir = (seg.h >= 0.0) ? 1.0 : -1.0;
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
                return seg.eval(t);
            }
        }
        throw std::out_of_range("DoPri87: interpolation time outside stored dense history");
    }

    [[nodiscard]] double interpolate_component(double t, int component) const
    {
        return interpolate(t)[component];
    }

    // ── Lifecycle hooks ──────────────────────────────────────────────────────

    void before_solve()
    {
        m_last = {};
        m_pending = {};
        m_dense_hist.clear();
    }

    void after_step(double /*t*/)
    {
        m_last = m_pending;
        m_dense_hist.push_back(m_pending);
    }

    // ── Step computation ─────────────────────────────────────────────────────
    //
    // On entry:  ws.k[0] = f(t, y)  (always set by solve_impl; has_fsal=false)
    // On exit:   ws.next = y_{n+1} (8th order)
    //            ws.error = error estimate
    //            ws.fsal  = f(t+h, y_{n+1}) for Hermite dense output
    //            m_pending = dense segment for this trial step

    template <typename RhsEval>
    void compute_step(double t, const Vec<N> &y, double h, RhsEval &&rhs, WorkspaceT &ws, SolverStats &stats)
    {
        // Save f₀ for Hermite dense output (ws.k[0] is stage 1, not overwritten,
        // but capturing it explicitly makes the intent clear)
        const Vec<N> &f0 = ws.k[0];  // reference — k[0] is never overwritten below

        // Precompute h·aᵢⱼ products to avoid repeated multiplications in stage sums
        const double ha21 = h * C::a21;
        const double ha31 = h * C::a31, ha32 = h * C::a32;
        const double ha41 = h * C::a41, ha43 = h * C::a43;
        const double ha51 = h * C::a51, ha53 = h * C::a53, ha54 = h * C::a54;
        const double ha61 = h * C::a61, ha64 = h * C::a64, ha65 = h * C::a65;
        const double ha71 = h * C::a71, ha74 = h * C::a74, ha75 = h * C::a75, ha76 = h * C::a76;
        const double ha81 = h * C::a81, ha84 = h * C::a84, ha85 = h * C::a85, ha86 = h * C::a86, ha87 = h * C::a87;
        const double ha91 = h * C::a91, ha94 = h * C::a94, ha95 = h * C::a95, ha96 = h * C::a96, ha97 = h * C::a97, ha98 = h * C::a98;
        const double ha101 = h * C::a101, ha104 = h * C::a104, ha105 = h * C::a105, ha106 = h * C::a106, ha107 = h * C::a107, ha108 = h * C::a108, ha109 = h * C::a109;
        const double ha111 = h * C::a111, ha114 = h * C::a114, ha115 = h * C::a115, ha116 = h * C::a116, ha117 = h * C::a117, ha118 = h * C::a118, ha119 = h * C::a119, ha1110 = h * C::a1110;
        const double ha121 = h * C::a121, ha124 = h * C::a124, ha125 = h * C::a125, ha126 = h * C::a126, ha127 = h * C::a127, ha128 = h * C::a128, ha129 = h * C::a129, ha1210 = h * C::a1210, ha1211 = h * C::a1211;
        const double ha131 = h * C::a131, ha134 = h * C::a134, ha135 = h * C::a135, ha136 = h * C::a136, ha137 = h * C::a137, ha138 = h * C::a138, ha139 = h * C::a139, ha1310 = h * C::a1310, ha1311 = h * C::a1311;

        // ── Stages 2–13 ─────────────────────────────────────────────────────
        // k[0] = f(t, y) was set by solve_impl; referenced by every stage below.

        rhs(t + C::c2 * h, y + ha21 * ws.k[0], ws.k[1]);
        ++stats.rhs_evals;

        rhs(t + C::c3 * h, y + ha31 * ws.k[0] + ha32 * ws.k[1], ws.k[2]);
        ++stats.rhs_evals;

        rhs(t + C::c4 * h, y + ha41 * ws.k[0] + ha43 * ws.k[2], ws.k[3]);
        ++stats.rhs_evals;

        rhs(t + C::c5 * h, y + ha51 * ws.k[0] + ha53 * ws.k[2] + ha54 * ws.k[3], ws.k[4]);
        ++stats.rhs_evals;

        rhs(t + C::c6 * h, y + ha61 * ws.k[0] + ha64 * ws.k[3] + ha65 * ws.k[4], ws.k[5]);
        ++stats.rhs_evals;

        rhs(t + C::c7 * h, y + ha71 * ws.k[0] + ha74 * ws.k[3] + ha75 * ws.k[4] + ha76 * ws.k[5], ws.k[6]);
        ++stats.rhs_evals;

        rhs(t + C::c8 * h, y + ha81 * ws.k[0] + ha84 * ws.k[3] + ha85 * ws.k[4] + ha86 * ws.k[5] + ha87 * ws.k[6], ws.k[7]);
        ++stats.rhs_evals;

        rhs(t + C::c9 * h, y + ha91 * ws.k[0] + ha94 * ws.k[3] + ha95 * ws.k[4] + ha96 * ws.k[5] + ha97 * ws.k[6] + ha98 * ws.k[7], ws.k[8]);
        ++stats.rhs_evals;

        rhs(t + C::c10 * h, y + ha101 * ws.k[0] + ha104 * ws.k[3] + ha105 * ws.k[4] + ha106 * ws.k[5] + ha107 * ws.k[6] + ha108 * ws.k[7] + ha109 * ws.k[8], ws.k[9]);
        ++stats.rhs_evals;

        rhs(t + C::c11 * h, y + ha111 * ws.k[0] + ha114 * ws.k[3] + ha115 * ws.k[4] + ha116 * ws.k[5] + ha117 * ws.k[6] + ha118 * ws.k[7] + ha119 * ws.k[8] + ha1110 * ws.k[9], ws.k[10]);
        ++stats.rhs_evals;

        // Stage 12 (c12 = 1.0)
        rhs(t + h, y + ha121 * ws.k[0] + ha124 * ws.k[3] + ha125 * ws.k[4] + ha126 * ws.k[5] + ha127 * ws.k[6] + ha128 * ws.k[7] + ha129 * ws.k[8] + ha1210 * ws.k[9] + ha1211 * ws.k[10], ws.k[11]);
        ++stats.rhs_evals;

        // Stage 13 (c13 = 1.0; a1312 = 0 — does NOT use ws.k[11])
        rhs(t + h, y + ha131 * ws.k[0] + ha134 * ws.k[3] + ha135 * ws.k[4] + ha136 * ws.k[5] + ha137 * ws.k[6] + ha138 * ws.k[7] + ha139 * ws.k[8] + ha1310 * ws.k[9] + ha1311 * ws.k[10], ws.k[12]);
        ++stats.rhs_evals;

        // ── 8th-order solution ───────────────────────────────────────────────
        ws.next = y + h * (C::b1 * ws.k[0] + C::b6 * ws.k[5] + C::b7 * ws.k[6] + C::b8 * ws.k[7] + C::b9 * ws.k[8] + C::b10 * ws.k[9] + C::b11 * ws.k[10] + C::b12 * ws.k[11] + C::b13 * ws.k[12]);

        // ── Error estimate h·(b−d)·k ─────────────────────────────────────────
        // e₁₃ = b₁₃ (d₁₃ = 0); all other eᵢ = bᵢ − dᵢ
        ws.error = h * (C::e1 * ws.k[0] + C::e6 * ws.k[5] + C::e7 * ws.k[6] + C::e8 * ws.k[7] + C::e9 * ws.k[8] + C::e10 * ws.k[9] + C::e11 * ws.k[10] + C::e12 * ws.k[11] + C::e13 * ws.k[12]);

        // ── Dense output: Hermite cubic from f₀ and f₁ ──────────────────────
        // One extra RHS evaluation per step; stored in ws.fsal (has_fsal=false
        // so solve_impl never reads ws.fsal as a stage vector).
        // The segment is only committed in after_step() if the step is accepted.
        rhs(t + h, ws.next, ws.fsal);
        ++stats.rhs_evals;

        m_pending = hermite_segment<N>(t, y, h, ws.next, f0, ws.fsal);
    }

  private:
    DenseSegment<N> m_last{};
    DenseSegment<N> m_pending{};
    std::vector<DenseSegment<N>> m_dense_hist{};
};

}  // namespace DES