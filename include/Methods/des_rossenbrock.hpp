#pragma once

/*  des_rossenbrock.hpp  –  DES namespace
 *
 *  Rosenbrock4 — GRK4A Rosenbrock method for stiff ODEs and DDEs.
 *
 *  This implementation uses the transformed Rosenbrock form
 *
 *      W = (1/(γ h)) I - J(t_n, y_n),
 *
 *  where J is the Jacobian with respect to the current state.  The same
 *  matrix W is factorized once and reused for all four stage solves in a step.
 *
 *  The stage vectors k_i stored in ws.k are O(h) state increments, not slope
 *  vectors.  Accordingly, stage states are formed as y + Σ a_ij k_j (no extra
 *  factor of h), and both the step update and embedded error estimate are plain
 *  linear combinations of the k_i.
 *
 *  References
 *  ──────────
 *  • P. Kaps, P. Rentrop.
 *    Generalized Runge-Kutta methods of order four with stepsize control for
 *    stiff ordinary differential equations.
 *    Numer. Math. 33 (1979) 55–68.
 *
 *  • E. Hairer, G. Wanner.
 *    Solving Ordinary Differential Equations II — Stiff and
 *    Differential-Algebraic Problems.
 *    2nd ed., Springer, 1996.
 *
 *  Properties
 *  ──────────
 *  • 4 stages, order 4, embedded order 3 for adaptive step-size control
 *  • A-stable (GRK4A is not L-stable)
 *  • Linearly implicit: one LU factorization per attempted step, reused for all
 *    four stages
 *  • Jacobian support:
 *      – Analytical: provide sys.jacobian(t, y, J) → used automatically
 *      – Finite-difference fallback when no .jacobian member is present
 *  • Dense output: generic Hermite cubic segment built from
 *      (t_n, y_n, f_n) and (t_{n+1}, y_{n+1}, f_{n+1})
 *
 *  Stage equations in the implemented form
 *  ───────────────────────────────────────
 *  With W = (1/(γ h)) I - J and J = ∂f/∂y evaluated at (t_n, y_n):
 *
 *      W k1 = f(t_n, y_n)
 *
 *      Y2   = y_n + a21 k1
 *      W k2 = f(t_n + c2 h, Y2) + (g21 / h) k1
 *
 *      Y3   = y_n + a31 k1 + a32 k2
 *      W k3 = f(t_n + c3 h, Y3) + (g31 / h) k1 + (g32 / h) k2
 *
 *      Y4   = Y3   [GRK4A has a4 = a3 and c4 = c3]
 *      W k4 = f(t_n + c3 h, Y3)
 *             + (g41 / h) k1 + (g42 / h) k2 + (g43 / h) k3
 *
 *  Step update and error estimate
 *  ──────────────────────────────
 *      y_{n+1} = y_n + b1 k1 + b2 k2 + b3 k3 + b4 k4
 *
 *      err     = e1 k1 + e2 k2 + e3 k3 + e4 k4
 *
 *  Implementation notes
 *  ────────────────────
 *  • compute_step() receives f(t_n, y_n) in ws.k[0] on entry and overwrites
 *    ws.k[0..3] with the four Rosenbrock stage increments.
 *  • Stage 4 reuses the stage-3 RHS evaluation because Y4 = Y3 and c4 = c3 in
 *    this GRK4A formulation, so there is no extra RHS call for stage 4.
 *  • Dense output here is the library’s generic Hermite cubic interpolant; it
 *    is not a Rosenbrock-specific stiff-aware dense output formula.
 *
 *  DDE usage
 *  ─────────
 *  For DDEs, J is the partial derivative with respect to the current state y.
 *  Delayed/history values are treated as frozen during each step and during
 *  finite-difference Jacobian construction.  This matches the rhs callback
 *  contract used by the base adaptive solver.
 */

#include "des_adaptive.hpp"
#include "des_dense_output.hpp"

#include <Eigen/LU>

#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace DES {

// ---------------------------------------------------------------------------
// HasJacobian<System, N>
//
// True when System has a member:
//   void jacobian(double t, const Vec<N>&, Eigen::Matrix<double,N,N>&)
// ---------------------------------------------------------------------------

template <typename System, int N, typename = void>
struct HasJacobian : std::false_type {};

template <typename System, int N>
struct HasJacobian<System, N, std::void_t<decltype(std::declval<System &>().jacobian(std::declval<double>(), std::declval<const Vec<N> &>(), std::declval<Eigen::Matrix<double, N, N> &>()))>> : std::true_type {};

// ---------------------------------------------------------------------------
// Rosenbrock4<N, HistoryPoints>
// ---------------------------------------------------------------------------

template <int N, int HistoryPoints = 1000>
class Rosenbrock4 : public AdaptiveDES<Rosenbrock4<N, HistoryPoints>, N, HistoryPoints, 4> {
    using Base = AdaptiveDES<Rosenbrock4<N, HistoryPoints>, N, HistoryPoints, 4>;
    using JacMat = Eigen::Matrix<double, N, N>;
    using LU_t = Eigen::PartialPivLU<JacMat>;
    using WorkspaceT = Workspace<N, 4>;

    // ── GRK4A coefficients (Kaps & Rentrop 1979) in transformed W-form ─────
    struct Coeff {
        // Diagonal shift: W = (1/γh)·I − J
        static constexpr double gamma = 0.395;

        // Stage y-arguments: Yᵢ = y + Σⱼ<ᵢ aᵢⱼ kⱼ
        // The kᵢ in this implementation are O(h) increments, so there is no
        // extra factor of h in the stage states, final update, or error estimate.
        static constexpr double a21 = 0.438;
        static constexpr double a31 = 0.796920457938;
        static constexpr double a32 = 0.0730795420615;
        // GRK4A: a₄ = a₃  ⟹ Y₄ = Y₃

        // Corresponding abscissae c_i = Σ_j a_ij
        static constexpr double c2 = a21;        // 0.438
        static constexpr double c3 = a31 + a32;  // 0.87

        // Off-diagonal Γᵢⱼ (divided by h in stage equations)
        static constexpr double g21 = -0.767672395484;
        static constexpr double g31 = -0.851675323742;
        static constexpr double g32 = 0.522967289188;
        static constexpr double g41 = 0.288463109545;
        static constexpr double g42 = 0.0880214273381;
        static constexpr double g43 = -0.337389840627;

        // 4th-order solution weights
        static constexpr double b1 = 0.199293275701;
        static constexpr double b2 = 0.482645235674;
        static constexpr double b3 = 0.0680614886256;
        static constexpr double b4 = 0.25;

        // Embedded 3rd-order weights: b̂ = [0.346325833758, 0.285693175712, 0.367980990530, 0]
        // Error = b − b̂
        static constexpr double e1 = -0.147032558057;
        static constexpr double e2 = 0.196952059962;
        static constexpr double e3 = -0.2999195019044;
        static constexpr double e4 = 0.25;
    };

  public:
    // Finite-difference epsilon for Jacobian approximation.
    // Perturbation for component j: fd_eps * max(|y[j]|, 1.0)
    double fd_eps = 1.0e-7;

    Rosenbrock4()
    {
        this->options.controller.kind = ControllerKind::PI;
        this->options.controller.safety = 0.9;
        this->options.controller.min_factor = 0.2;
        this->options.controller.max_factor = 6.0;
        this->options.h_max = 1.0;
    }

    // ── CRTP capability queries ─────────────────────────────────────────────

    [[nodiscard]] static constexpr int method_order()
    {
        return 4;
    }
    [[nodiscard]] static constexpr int adaptive_order()
    {
        return 3;
    }
    [[nodiscard]] static constexpr bool has_fsal()
    {
        return false;
    }

    // ── Dense output (generic Hermite cubic, O(h⁴)) ────────────────────────

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
            throw std::out_of_range("Rosenbrock4: dense history index out of range");
        }
        return m_dense_hist[static_cast<std::size_t>(i)];
    }

    [[nodiscard]] Vec<N> interpolate(double t) const
    {
        const int n = dense_history_size();
        if (n == 0)
        {
            throw std::out_of_range("Rosenbrock4: no dense segments stored");
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
        throw std::out_of_range("Rosenbrock4: interpolation time outside dense history");
    }

    [[nodiscard]] double interpolate_component(double t, int component) const
    {
        return interpolate(t)[component];
    }

    // ── solve() overloads ─────────────────────────────────────────────────────
    //
    // These shadow the base-class overloads so that setup_jacobian() is called
    // exactly once per solve, before the integration loop begins.

    // ODE — no observer
    template <typename System>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys)
    {
        setup_jacobian(sys);
        typename Base::NoOpObserver obs;
        return Base::solve(y, t0, t1, sys, obs);
    }

    // ODE — with observer
    template <typename System, typename Observer>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, Observer &&obs)
    {
        setup_jacobian(sys);
        return Base::solve(y, t0, t1, sys, std::forward<Observer>(obs));
    }

    // DDE — no observer
    template <typename System>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, typename Base::DelayHistoryStorage &dh)
    {
        setup_jacobian(sys);
        typename Base::NoOpObserver obs;
        return Base::solve(y, t0, t1, sys, dh, obs);
    }

    // DDE — with observer
    template <typename System, typename Observer>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, typename Base::DelayHistoryStorage &dh, Observer &&obs)
    {
        setup_jacobian(sys);
        return Base::solve(y, t0, t1, sys, dh, std::forward<Observer>(obs));
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
    // On entry: ws.k[0] = f(t, y)  (set by solve_impl; has_fsal = false)
    //
    // The method overwrites ws.k[0..3] with Rosenbrock stage increments k₁..k₄
    // in transformed W-form.  They are not raw function evaluations.
    // ws.fsal stores f(t+h, y_{n+1}) for the Hermite dense segment.
    //
    // Jacobian J = ∂f/∂y is evaluated at (t, y).  For DDEs, delayed/history
    // values are frozen during the step; the rhs lambda captures the query
    // window accordingly, so perturbing y for FD Jacobians does not change the
    // delay lookup horizon.

    template <typename RhsEval>
    void compute_step(double t, const Vec<N> &y, double h, RhsEval &&rhs, WorkspaceT &ws, SolverStats &stats)
    {
        using Cf = Coeff;

        // f₀ = f(t, y) — already in ws.k[0] on entry.
        // In this Rosenbrock formulation the stage vectors kᵢ are O(h) increments,
        // not slope vectors. Therefore stage states use y + Σ aᵢⱼ kⱼ (no extra h), and
        // the final update/error estimate are also plain linear combinations of kᵢ.
        // Save as a const reference; we need it for:
        //   (a) the right-hand side of the W·k₁ = f₀ equation
        //   (b) the FD Jacobian column differences
        //   (c) the Hermite f₀ for dense output
        const Vec<N> f0 = ws.k[0];

        // ── Jacobian J = ∂f/∂y at (t, y) ────────────────────────────────────
        JacMat J;
        if (m_jac_fn)
        {
            m_jac_fn(t, y, J);
        }
        else
        {
            compute_jac_fd(t, y, f0, rhs, J, stats);
        }

        // ── Build and LU-factor W = (1/γh)·I − J ────────────────────────────
        // One factorization is performed per attempted step and reused for all stages.
        const double inv_gh = 1.0 / (Cf::gamma * h);
        JacMat W = -J;
        W.diagonal().array() += inv_gh;  // W += (1/γh)·I
        const LU_t lu(W);

        // ── Stage 1: W k₁ = f(t, y) ─────────────────────────────────────────
        ws.k[0] = lu.solve(f0);

        // ── Stage 2: W k₂ = f(t + c₂h, Y₂) + (Γ₂₁/h) k₁ ──────────────────
        {
            Vec<N> f2;
            rhs(t + Cf::c2 * h, y + Cf::a21 * ws.k[0], f2);
            ++stats.rhs_evals;
            ws.k[1] = lu.solve(f2 + (Cf::g21 / h) * ws.k[0]);
        }

        // ── Stage 3: W k₃ = f(t + c₃h, Y₃) + (Γ₃₁/h) k₁ + (Γ₃₂/h) k₂ ────
        const Vec<N> Y3 = y + (Cf::a31 * ws.k[0] + Cf::a32 * ws.k[1]);
        Vec<N> f3;
        {
            rhs(t + Cf::c3 * h, Y3, f3);
            ++stats.rhs_evals;
            ws.k[2] = lu.solve(f3 + (Cf::g31 / h) * ws.k[0] + (Cf::g32 / h) * ws.k[1]);
        }

        // ── Stage 4: W k₄ = f(t + c₃h, Y₃) + (Γ₄₁/h) k₁ + (Γ₄₂/h) k₂ + (Γ₄₃/h) k₃ ────
        // GRK4A property: a₄ = a₃ ⟹ Y₄ = Y₃, so the stage-4 y-argument is
        // identical to stage 3 and we can reuse f3 without another RHS call.
        {
            ws.k[3] = lu.solve(f3 + (Cf::g41 / h) * ws.k[0] + (Cf::g42 / h) * ws.k[1] + (Cf::g43 / h) * ws.k[2]);
        }

        // ── 4th-order solution ───────────────────────────────────────────────
        ws.next = y + (Cf::b1 * ws.k[0] + Cf::b2 * ws.k[1] + Cf::b3 * ws.k[2] + Cf::b4 * ws.k[3]);

        // ── Error estimate: e₁k₁ + e₂k₂ + e₃k₃ + e₄k₄ ─────────────────────
        ws.error = (Cf::e1 * ws.k[0] + Cf::e2 * ws.k[1] + Cf::e3 * ws.k[2] + Cf::e4 * ws.k[3]);

        // ── Dense output: generic Hermite cubic from f₀ and f₁ = f(t+h, y_new) ──────
        // This is library-level dense output, not a Rosenbrock-specific interpolant.
        // f₁ is stored in ws.fsal (has_fsal=false, so solve_impl never reads it
        // as a recycled stage).  The pending segment is committed only on
        // acceptance, in after_step().
        rhs(t + h, ws.next, ws.fsal);
        ++stats.rhs_evals;

        m_pending = hermite_segment<N>(t, y, h, ws.next, f0, ws.fsal);
    }

  private:
    DenseSegment<N> m_last{};
    DenseSegment<N> m_pending{};
    std::vector<DenseSegment<N>> m_dense_hist{};

    // Type-erased Jacobian function.  Null ⟹ use finite differences.
    // Set once per solve() call by setup_jacobian(); valid for its duration.
    std::function<void(double, const Vec<N> &, JacMat &)> m_jac_fn;

    // ── Jacobian setup ───────────────────────────────────────────────────────
    //
    // Detects at compile time whether System provides .jacobian().
    // If so, captures it by reference (the reference is valid for the duration
    // of the solve() call that owns this solver).

    template <typename System>
    void setup_jacobian(System &sys)
    {
        if constexpr (HasJacobian<System, N>::value)
        {
            m_jac_fn = [&sys](double t, const Vec<N> &y, JacMat &J) { sys.jacobian(t, y, J); };
        }
        else
        {
            m_jac_fn = nullptr;  // compute_step will use finite differences
        }
    }

    // ── Finite-difference Jacobian ───────────────────────────────────────────
    //
    // Forward-difference column by column:
    //   J[:,j] ≈ (f(t, y + ε eⱼ) − f(t, y)) / ε
    //   ε = fd_eps · max(|y[j]|, 1)
    //
    // Cost: N extra RHS evaluations counted in stats.rhs_evals.
    // For DDEs: the rhs lambda fixes max_query_time = t (step start), so
    // perturbing y does not change the delay query window — correct.

    template <typename RhsEval>
    void compute_jac_fd(double t, const Vec<N> &y, const Vec<N> &f0, RhsEval &rhs, JacMat &J, SolverStats &stats)
    {
        Vec<N> y_pert, f_pert;
        for (int j = 0; j < N; ++j)
        {
            const double eps_j = fd_eps * std::max(std::abs(y[j]), 1.0);
            y_pert = y;
            y_pert[j] += eps_j;
            rhs(t, y_pert, f_pert);
            ++stats.rhs_evals;
            J.col(j) = (f_pert - f0) * (1.0 / eps_j);
        }
    }
};

}  // namespace DES
