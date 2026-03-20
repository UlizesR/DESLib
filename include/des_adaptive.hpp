#pragma once

#include "DES.hpp"
#include "history.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace DES {

// ---------------------------------------------------------------------------
// DelayHistoryView<N>
//
// A read-only, causality-enforcing view into a DDE History object.
// Passed to the user's RHS as the third argument for DDE systems.
//
// Causality rule: query_time must not exceed the start of the current step
// (max_query_time_).  Violations throw std::logic_error.
//
// State-dependent delays: the user's RHS can call at_time(i, alpha(t,y))
// directly, where alpha is any function of the current state — no solver
// changes are needed to support state-dependent delays on the RHS side.
// ---------------------------------------------------------------------------

template <int N>
class DelayHistoryView {
  public:
    using ScalarHistory = DES::History<double, double>;

    DelayHistoryView() = default;

    DelayHistoryView(const ScalarHistory *history, double max_query_time) noexcept
        : m_history(history)
        , m_max_qt(max_query_time)
    {}

    // Primary access: variable i at time query_time.
    [[nodiscard]] double operator()(std::size_t i, double query_time) const
    {
        check_causality(query_time);
        return m_history->at_time(query_time, i);
    }

    // Named alias for readability in user code
    [[nodiscard]] double at_time(std::size_t i, double t) const
    {
        return (*this)(i, t);
    }

    // Return the full N-dimensional state vector at a past time
    [[nodiscard]] Vec<N> state(double t) const
    {
        Vec<N> out;
        for (int i = 0; i < N; ++i)
        {
            out[i] = (*this)(static_cast<std::size_t>(i), t);
        }
        return out;
    }

    // Convenience: state at (now − tau)
    [[nodiscard]] Vec<N> delay(double now, double tau) const
    {
        return state(now - tau);
    }

    [[nodiscard]] double max_query_time() const noexcept
    {
        return m_max_qt;
    }

  private:
    const ScalarHistory *m_history = nullptr;
    double m_max_qt = -std::numeric_limits<double>::infinity();

    void check_causality(double query_time) const
    {
        if (!m_history)
        {
            throw std::logic_error("DelayHistoryView: no history attached");
        }
        const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(query_time), std::abs(m_max_qt)));
        if (query_time > m_max_qt + eps)
        {
            throw std::logic_error("DelayHistoryView: query time lies inside the current step. "
                                   "Reduce the step size or set Options::min_delay correctly.");
        }
    }
};

// ---------------------------------------------------------------------------
// AdaptiveDES<Derived, N, HistoryPoints, MaxStages>
//
// CRTP base class for explicit adaptive Runge–Kutta solvers.
//
// The Derived class MUST implement:
//   static constexpr int  method_order()
//   static constexpr int  adaptive_order()
//   static constexpr bool has_fsal()
//   void compute_step(double t, const Vec<N>&, double h,
//                     RhsEval&&, Workspace<N,MaxStages>&, SolverStats&)
//
// Optional hooks Derived MAY override:
//   void before_solve()
//   void after_step(double t)
//   void after_capture(double t)
//   void after_solve()
//
// Features
// ────────
//   • ODE and DDE right-hand sides (auto-detected from System signature)
//   • PI / Gustafsson / Integral step-size controllers
//   • FSAL optimisation
//   • Adaptive initial step selection (Hairer & Wanner §II.4)
//   • DDE breaking-point schedule (pre-computed from declared delays)
//   • Zero-crossing event detection with dense-output bisection
//   • Multiple independent delays (expose at_time on DelayHistoryView)
//   • Observer callbacks (adaptive or uniform-grid output)
// ---------------------------------------------------------------------------

template <typename Derived, int N, int HistoryPoints = 5000, int MaxStages = 16>
class AdaptiveDES {
  public:
    using DelayHistoryStorage = DES::History<double, double>;

    // -----------------------------------------------------------------------
    // EventSpec — describes one scalar event function g(t, y) = 0
    // -----------------------------------------------------------------------

    struct EventSpec {
        // Returns 0 at the event; sign change triggers detection
        std::function<double(double, const Vec<N> &)> func;

        bool terminal = false;  // stop integration when triggered
        int direction = 0;      // 0=any, +1=rising (−→+), −1=falling (+→−)

        double location_tol = 1.0e-10;  // bisection stopping width
        int max_bisect_iters = 60;      // safety cap
    };

    // -----------------------------------------------------------------------
    // Options
    // -----------------------------------------------------------------------

    struct Options {
        // Tolerances
        double rtol = 1.0e-9;
        double atol = 1.0e-9;
        std::array<double, N> atol_vec{};
        bool use_vector_atol = false;

        // Step-size bounds
        double h_init = 0.0;  // 0 → auto-select
        double h_min = 1.0e-12;
        double h_max = 1.0;

        long max_steps = 1'000'000;
        long integrity_check_stride = 100;  // 0 → disabled

        // Output recording
        bool save_history = true;
        bool uniform_output = false;
        int output_points = HistoryPoints;

        // DDE: cap step size so α(t,y) stays before step start
        double min_delay = std::numeric_limits<double>::infinity();

        ErrorNorm error_norm = ErrorNorm::Rms;
        StepController controller{};

        // ── Breaking-point schedule (Guglielmi & Hairer §1.1.1) ──────────
        // Pre-compute mandatory mesh points from declared constant delays.
        // The solver enforces step boundaries at each breaking point and
        // invalidates FSAL there (derivative discontinuity).
        bool detect_breaking_points = true;
        int breaking_point_levels = 5;        // j = 1 … levels
        std::vector<double> declared_delays;  // constant delays

        // ── Events (zero-crossing detection) ─────────────────────────────
        // Requires dense output (HasLastDenseStep<Derived>).
        std::vector<EventSpec> events;
    };

    // -----------------------------------------------------------------------
    // OutputHistory — recorded solution points
    // -----------------------------------------------------------------------

    struct OutputHistory {
        std::vector<double> t{};
        std::vector<double> h{};
        std::vector<double> error{};
        std::vector<Vec<N>> y{};
    };

    Options options{};

    AdaptiveDES() = default;

    [[nodiscard]] const OutputHistory &history() const noexcept
    {
        return m_hist;
    }
    [[nodiscard]] const SolverStats &stats() const noexcept
    {
        return m_stats;
    }
    [[nodiscard]] int history_size() const noexcept
    {
        return static_cast<int>(m_hist.t.size());
    }

    // Default no-op lifecycle hooks (Derived may override)
    void before_solve()
    {}
    void after_step(double)
    {}
    void after_capture(double)
    {}
    void after_solve()
    {}

    // -----------------------------------------------------------------------
    // solve() overloads — ODE, DDE, with/without observer
    // -----------------------------------------------------------------------

    struct NoOpObserver {
        void operator()(double, const Vec<N> &, const Vec<N> &) const
        {}
    };

    // ODE variants
    template <typename System>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys)
    {
        NoOpObserver obs;
        return solve_impl(y, t0, t1, sys, nullptr, obs);
    }

    template <typename System, typename Observer>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, Observer &&obs)
    {
        return solve_impl(y, t0, t1, sys, nullptr, std::forward<Observer>(obs));
    }

    // DDE variants
    template <typename System>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, DelayHistoryStorage &dh)
    {
        NoOpObserver obs;
        return solve_impl(y, t0, t1, sys, &dh, obs);
    }

    template <typename System, typename Observer>
    SolveResult solve(Vec<N> &y, double t0, double t1, System &sys, DelayHistoryStorage &dh, Observer &&obs)
    {
        return solve_impl(y, t0, t1, sys, &dh, std::forward<Observer>(obs));
    }

  protected:
    Workspace<N, MaxStages> m_ws{};
    SolverStats m_stats{};

    // ── Order queries ───────────────────────────────────────────────────────

    [[nodiscard]] static constexpr int method_order()
    {
        if constexpr (HasMethodOrder<Derived>::value)
        {
            return Derived::method_order();
        }
        return 1;
    }

    [[nodiscard]] static constexpr int adaptive_order()
    {
        if constexpr (HasAdaptiveOrder<Derived>::value)
        {
            return Derived::adaptive_order();
        }
        return std::max(0, method_order() - 1);
    }

    [[nodiscard]] static constexpr bool has_fsal()
    {
        if constexpr (HasFsal<Derived>::value)
        {
            return Derived::has_fsal();
        }
        return false;
    }

    // ── Error norm helpers (Eigen-vectorised) ───────────────────────────────

    [[nodiscard]] double component_atol(int i) const
    {
        return options.use_vector_atol ? options.atol_vec[static_cast<std::size_t>(i)] : options.atol;
    }

    // v.array().isFinite().all() — uses Eigen's SIMD path
    [[nodiscard]] bool is_finite(const Vec<N> &v) const
    {
        return v.array().isFinite().all();
    }

  private:
    OutputHistory m_hist{};

    // ── System signature detection ──────────────────────────────────────────

    template <typename System>
    static constexpr bool supports_ode_rhs()
    {
        return std::is_invocable_v<System &, double, const Vec<N> &, Vec<N> &>;
    }

    template <typename System>
    static constexpr bool supports_dde_rhs()
    {
        return std::is_invocable_v<System &, double, const Vec<N> &, const DelayHistoryView<N> &, Vec<N> &>;
    }

    // ── Utilities ───────────────────────────────────────────────────────────

    static void fill_zero(Vec<N> &v)
    {
        v.setZero();
    }

    // Eigen column vectors are contiguous — avoid per-element copy
    static std::vector<double> to_std_vector(const Vec<N> &v)
    {
        return std::vector<double>(v.data(), v.data() + N);
    }

    [[nodiscard]] double declared_min_delay(const DelayHistoryStorage *dh) const
    {
        if (!dh)
        {
            return std::numeric_limits<double>::infinity();
        }
        const double from_hist = dh->min_delay();
        return std::isfinite(options.min_delay) ? std::min(options.min_delay, from_hist) : from_hist;
    }

    // ── RHS dispatch ─────────────────────────────────────────────────────────
    //
    // Routes to the DDE or ODE calling convention based on the System type.
    // Template parameter is `System` — NEVER `s` (was a pre-existing typo).

    template <typename System>
    void call_rhs(double t, const Vec<N> &y, System &sys, Vec<N> &dydt, const DelayHistoryStorage *dh, double max_query_time)
    {
        // DDE path: system callable as f(t, y, history_view, dydt)
        if constexpr (supports_dde_rhs<System>())
        {
            if (dh)
            {
                const DelayHistoryView<N> view(dh, max_query_time);
                sys(t, y, view, dydt);
                return;
            }
        }

        // ODE path: system callable as f(t, y, dydt)
        if constexpr (supports_ode_rhs<System>())
        {
            sys(t, y, dydt);
            return;
        }

        static_assert(supports_ode_rhs<System>() || supports_dde_rhs<System>(), "System must be callable as f(t,y,dydt) "
                                                                                "or f(t,y,history_view,dydt)");
    }

    // ── Workspace and output storage reset ─────────────────────────────────

    void reset_workspace()
    {
        for (auto &k : m_ws.k)
        {
            k.setZero();
        }
        m_ws.next.setZero();
        m_ws.error.setZero();
        m_ws.fsal.setZero();
    }

    void reset_output_storage()
    {
        m_hist.t.clear();
        m_hist.h.clear();
        m_hist.error.clear();
        m_hist.y.clear();

        if (options.save_history && options.uniform_output && options.output_points > 0)
        {
            const auto n = static_cast<std::size_t>(options.output_points);
            m_hist.t.reserve(n);
            m_hist.h.reserve(n);
            m_hist.error.reserve(n);
            m_hist.y.reserve(n);
        }
    }

    // ── Recording and observer dispatch ────────────────────────────────────

    void record(double t, double h, double err, const Vec<N> &y)
    {
        if (!options.save_history)
        {
            return;
        }
        m_hist.t.push_back(t);
        m_hist.h.push_back(h);
        m_hist.error.push_back(err);
        m_hist.y.push_back(y);
        ++m_stats.history_writes;
        static_cast<Derived *>(this)->after_capture(t);
    }

    template <typename Observer>
    void notify(Observer &obs, double t, const Vec<N> &y, const Vec<N> &err)
    {
        if constexpr (std::is_invocable_v<Observer, double, const Vec<N> &, const Vec<N> &, const SolverStats &>)
        {
            obs(t, y, err, m_stats);
        }
        else if constexpr (std::is_invocable_v<Observer, double, const Vec<N> &, const Vec<N> &>)
        {
            obs(t, y, err);
        }
        else
        {
            static_assert(std::is_invocable_v<Observer, double, const Vec<N> &, const Vec<N> &, const SolverStats &> || std::is_invocable_v<Observer, double, const Vec<N> &, const Vec<N> &>, "Observer must be callable as (t,y,err) or (t,y,err,stats)");
        }
    }

    // ── Scaled error norms (Eigen array ops for SIMD) ──────────────────────

    // Build sc[i] = atol[i] + rtol·max(|y0[i]|, |y1[i]|)
    [[nodiscard]] Vec<N> scale_vec(const Vec<N> &y0, const Vec<N> &y1) const
    {
        Vec<N> sc;
        for (int i = 0; i < N; ++i)
        {
            sc[i] = component_atol(i) + options.rtol * std::max(std::abs(y0[i]), std::abs(y1[i]));
        }
        return sc;
    }

    [[nodiscard]] double scaled_error(const Vec<N> &y0, const Vec<N> &y1, const Vec<N> &err) const
    {
        if (!is_finite(err))
        {
            return std::numeric_limits<double>::infinity();
        }

        const Vec<N> sc = scale_vec(y0, y1);
        if (!(sc.array() > 0.0).all() || !sc.array().isFinite().all())
        {
            return std::numeric_limits<double>::infinity();
        }

        const Vec<N> q = err.array() / sc.array();  // Eigen element-wise

        if (options.error_norm == ErrorNorm::Infinity)
        {
            return q.array().abs().maxCoeff();
        }
        return std::sqrt(q.squaredNorm() / static_cast<double>(N));
    }

    [[nodiscard]] double weighted_norm(const Vec<N> &v, const Vec<N> &ref) const
    {
        Vec<N> sc;
        for (int i = 0; i < N; ++i)
        {
            sc[i] = component_atol(i) + options.rtol * std::abs(ref[i]);
        }
        if (!(sc.array() > 0.0).all() || !sc.array().isFinite().all())
        {
            return std::numeric_limits<double>::infinity();
        }
        const Vec<N> q = v.array() / sc.array();
        if (options.error_norm == ErrorNorm::Infinity)
        {
            return q.array().abs().maxCoeff();
        }
        return std::sqrt(q.squaredNorm() / static_cast<double>(N));
    }

    // ── Automatic initial step (Hairer & Wanner §II.4) ─────────────────────

    template <typename System>
    double choose_initial_step(double t, const Vec<N> &y, double t1, double dir, System &sys, DelayHistoryStorage *dh, bool &have_rhs)
    {
        if (!have_rhs)
        {
            call_rhs(t, y, sys, m_ws.k[0], dh, t);
            ++m_stats.rhs_evals;
            if (!is_finite(m_ws.k[0]))
            {
                return options.h_min;
            }
            have_rhs = true;
        }

        const double d0 = weighted_norm(y, y);
        const double d1 = weighted_norm(m_ws.k[0], y);

        double h0 = 1.0e-6;
        if (std::isfinite(d0) && std::isfinite(d1) && d0 >= 1.0e-5 && d1 >= 1.0e-5)
        {
            h0 = 0.01 * d0 / d1;
        }
        h0 = std::clamp(std::min(h0, std::abs(t1 - t)), options.h_min, options.h_max);

        const double md = declared_min_delay(dh);
        if (dh && std::isfinite(md))
        {
            h0 = std::min(h0, md);
        }

        // Euler trial step to estimate curvature
        Vec<N> f_trial;
        call_rhs(t + dir * h0, y + (dir * h0) * m_ws.k[0], sys, f_trial, dh, t);
        ++m_stats.rhs_evals;

        if (!is_finite(f_trial))
        {
            return h0;
        }

        const double d2 = [&] {
            const double raw = weighted_norm(f_trial - m_ws.k[0], y);
            return (h0 > 0.0) ? raw / h0 : std::numeric_limits<double>::infinity();
        }();

        const double order = static_cast<double>(adaptive_order() + 1);
        const double denom = std::max(d1, d2);

        double h1 = (std::isfinite(denom) && denom > 1.0e-15) ? std::pow(0.01 / denom, 1.0 / order) : std::max(1.0e-6, h0 * 1.0e-3);

        double h = std::clamp(std::min({100.0 * h0, h1, std::abs(t1 - t)}), options.h_min, options.h_max);
        if (dh && std::isfinite(md))
        {
            h = std::min(h, md);
        }
        return h;
    }

    // ── Validation ──────────────────────────────────────────────────────────

    void validate_options() const
    {
        if (!(options.rtol > 0.0) || !(options.atol > 0.0))
        {
            throw std::invalid_argument("DES: rtol and atol must be positive");
        }
        if (!(options.h_min > 0.0) || !(options.h_max >= options.h_min))
        {
            throw std::invalid_argument("DES: invalid step-size bounds");
        }
        if (!(options.max_steps > 0))
        {
            throw std::invalid_argument("DES: max_steps must be positive");
        }
        if (!(options.controller.safety > 0.0))
        {
            throw std::invalid_argument("DES: controller safety must be positive");
        }
        if (!(options.controller.min_factor > 0.0) || !(options.controller.max_factor >= options.controller.min_factor))
        {
            throw std::invalid_argument("DES: invalid controller factor bounds");
        }
        if (options.use_vector_atol)
        {
            for (double a : options.atol_vec)
            {
                if (!(a > 0.0))
                {
                    throw std::invalid_argument("DES: vector atol entries must be positive");
                }
            }
        }
        if (!(method_order() > 0) || !(adaptive_order() >= 0))
        {
            throw std::invalid_argument("DES: derived solver reports invalid order");
        }
        if (options.uniform_output)
        {
            if (!(options.output_points > 0))
            {
                throw std::invalid_argument("DES: output_points must be positive");
            }
            if constexpr (!HasLastDenseStep<Derived>::value)
            {
                throw std::invalid_argument("DES: uniform_output requires dense-output support");
            }
        }
        if (std::isfinite(options.min_delay) && !(options.min_delay > 0.0))
        {
            throw std::invalid_argument("DES: min_delay must be positive when finite");
        }
        if (!options.events.empty())
        {
            if constexpr (!HasLastDenseStep<Derived>::value)
            {
                throw std::invalid_argument("DES: event detection requires dense-output support "
                                            "(e.g. DoPri54)");
            }
            for (const auto &ev : options.events)
            {
                if (!ev.func)
                {
                    throw std::invalid_argument("DES: EventSpec has null function");
                }
            }
        }
    }

    // ── Breaking-point schedule ─────────────────────────────────────────────
    //
    // For constant delay τ, derivative discontinuities propagate to
    // t₀ + j·τ, j = 1, 2, … (Guglielmi & Hairer §1.1.1).
    // We pre-compute these as mandatory mesh points.

    [[nodiscard]] std::vector<double> compute_bp_schedule(double t0, double t1, double dir, const DelayHistoryStorage *dh) const
    {
        if (!options.detect_breaking_points)
        {
            return {};
        }

        std::vector<double> sched;

        auto add_delays = [&](const std::vector<double> &delays) {
            for (double tau : delays)
            {
                if (!std::isfinite(tau) || tau <= 0.0)
                {
                    continue;
                }
                for (int k = 1; k <= options.breaking_point_levels; ++k)
                {
                    const double bp = t0 + dir * k * tau;
                    const double eps = 1.0e-14 * std::max(1.0, std::abs(t0));
                    const bool inside = (dir > 0.0) ? (bp > t0 + eps && bp < t1) : (bp < t0 - eps && bp > t1);
                    if (inside)
                    {
                        sched.push_back(bp);
                    }
                }
            }
        };

        add_delays(options.declared_delays);
        if (dh)
        {
            add_delays(dh->max_delays());
        }

        if (dir > 0.0)
        {
            std::sort(sched.begin(), sched.end());
        }
        else
        {
            std::sort(sched.begin(), sched.end(), std::greater<double>());
        }

        sched.erase(std::unique(sched.begin(), sched.end()), sched.end());
        return sched;
    }

    // ── Uniform-output flush ────────────────────────────────────────────────

    template <typename Observer>
    void flush_uniform(Observer &obs, double t1, double out_t0, double &next_out_t, double out_dt, int &next_out_i, double h, double err)
    {
        if constexpr (HasLastDenseStep<Derived>::value)
        {
            const auto &seg = static_cast<Derived *>(this)->last_dense_step();
            const double d = (t1 >= seg.t0) ? 1.0 : -1.0;

            while (next_out_i < options.output_points && seg.contains(next_out_t))
            {
                const Vec<N> val = seg.eval(next_out_t);
                record(next_out_t, h, err, val);
                notify(obs, next_out_t, val, m_ws.error);
                ++next_out_i;
                next_out_t = out_t0 + out_dt * static_cast<double>(next_out_i);
            }

            // Ensure t1 is always the final recorded point
            const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(t1), std::abs(seg.t0 + seg.h)));
            const bool at_end = (d > 0.0) ? (t1 <= seg.t0 + seg.h + eps) : (t1 >= seg.t0 + seg.h - eps);
            if (next_out_i == options.output_points - 1 && at_end)
            {
                const Vec<N> val = seg.eval(t1);
                record(t1, h, err, val);
                notify(obs, t1, val, m_ws.error);
                ++next_out_i;
            }
        }
    }

    // ── SolveResult factory ─────────────────────────────────────────────────

    [[nodiscard]] SolveResult make_result(SolveStatus st, double t, double h, double err) const
    {
        return SolveResult{st, t, h, err, static_cast<long>(m_hist.t.size()), -1, m_stats.breaking_points_crossed};
    }

    // ── Event detection on a single accepted step ───────────────────────────
    //
    // Evaluates all event functions at the new state; sign-changes trigger
    // bisection on the dense polynomial.  Returns the earliest TERMINAL event
    // as {t_event, y_event, event_index}, or nullopt if none.
    //
    // Non-terminal events are counted in m_stats but do not stop integration.

    using EventHit = std::tuple<double, Vec<N>, int>;

    [[nodiscard]] std::optional<EventHit> detect_events(double t_old, double t_new, const Vec<N> &y_new, std::vector<double> &g_prev)
    {
        if (options.events.empty())
        {
            return std::nullopt;
        }
        if constexpr (!HasLastDenseStep<Derived>::value)
        {
            return std::nullopt;
        }

        const auto &seg = static_cast<const Derived *>(this)->last_dense_step();
        const double dir = (t_new > t_old) ? 1.0 : -1.0;

        std::vector<EventHit> hits;

        for (int ei = 0; ei < static_cast<int>(options.events.size()); ++ei)
        {
            const auto &spec = options.events[static_cast<std::size_t>(ei)];
            const double g_new = spec.func(t_new, y_new);
            const double g_old = g_prev[static_cast<std::size_t>(ei)];
            const bool rising = g_old < 0.0 && g_new >= 0.0;
            const bool falling = g_old > 0.0 && g_new <= 0.0;

            bool triggered = false;
            switch (spec.direction)
            {
                case 0:
                    triggered = rising || falling;
                    break;
                case +1:
                    triggered = rising;
                    break;
                case -1:
                    triggered = falling;
                    break;
                default:
                    break;
            }

            if (triggered)
            {
                ++m_stats.events_triggered;

                // Bisect on dense polynomial to locate g = 0
                double la = t_old, lb = t_new;
                double ga = g_old, gb = g_new;

                for (int iter = 0; iter < spec.max_bisect_iters; ++iter)
                {
                    if (std::abs(lb - la) <= spec.location_tol)
                    {
                        break;
                    }
                    const double lm = 0.5 * (la + lb);
                    const double gm = spec.func(lm, seg.eval(lm));
                    if (ga * gm <= 0.0)
                    {
                        lb = lm;
                        gb = gm;
                    }
                    else
                    {
                        la = lm;
                        ga = gm;
                    }
                    ++m_stats.bisection_iters;
                }
                // Suppress unused variable warning in some paths
                (void)gb;

                hits.emplace_back(0.5 * (la + lb), seg.eval(0.5 * (la + lb)), ei);
            }

            g_prev[static_cast<std::size_t>(ei)] = g_new;
        }

        if (hits.empty())
        {
            return std::nullopt;
        }

        // Sort by event time in integration direction
        std::sort(hits.begin(), hits.end(), [dir](const EventHit &a, const EventHit &b) { return dir > 0.0 ? std::get<0>(a) < std::get<0>(b) : std::get<0>(a) > std::get<0>(b); });

        // Return earliest TERMINAL event
        for (auto &hit : hits)
        {
            const int ei = std::get<2>(hit);
            if (options.events[static_cast<std::size_t>(ei)].terminal)
            {
                return hit;
            }
        }
        return std::nullopt;
    }

    // ── Core integration loop ───────────────────────────────────────────────

    template <typename System, typename Observer>
    SolveResult solve_impl(Vec<N> &y, double t0, double t1, System &sys, DelayHistoryStorage *dh, Observer &&obs)
    {
        validate_options();
        reset_workspace();
        reset_output_storage();
        m_stats = {};

        if (!is_finite(y))
        {
            return make_result(SolveStatus::NonFiniteState, t0, 0.0, std::numeric_limits<double>::infinity());
        }

        static_cast<Derived *>(this)->before_solve();

        const double span = t1 - t0;
        if (dh && span < 0.0)
        {
            throw std::invalid_argument("DES: delay-history solve supports forward integration only");
        }

        const double dir = (span >= 0.0) ? 1.0 : -1.0;

        // ── Breaking-point schedule ────────────────────────────────────────
        const std::vector<double> bp_sched = compute_bp_schedule(t0, t1, dir, dh);
        std::size_t bp_idx = 0;

        // ── Record initial point ───────────────────────────────────────────
        record(t0, 0.0, 0.0, y);
        notify(obs, t0, y, m_ws.error);

        if (span == 0.0)
        {
            static_cast<Derived *>(this)->after_solve();
            return make_result(SolveStatus::Success, t0, 0.0, 0.0);
        }

        // ── Uniform-output grid ────────────────────────────────────────────
        const bool uniform = options.uniform_output;
        double out_dt = 0.0;
        double next_out = t0;
        int out_idx = 1;  // index 0 already recorded

        if (uniform)
        {
            if (options.output_points > 1)
            {
                out_dt = (t1 - t0) / static_cast<double>(options.output_points - 1);
                next_out = t0 + out_dt;
            }
            else
            {
                next_out = t1;
            }
        }

        // ── Event initial values ───────────────────────────────────────────
        std::vector<double> g_prev(options.events.size());
        for (std::size_t ei = 0; ei < options.events.size(); ++ei)
        {
            g_prev[ei] = options.events[ei].func(t0, y);
        }

        // ── Integration state ──────────────────────────────────────────────
        bool have_rhs = false;
        bool fsal_valid = false;
        bool dde_seeded = false;
        double err_norm = 0.0;
        ControllerState ctrl{};

        // ── Initial step size ─────────────────────────────────────────────
        const double md = declared_min_delay(dh);

        double h_abs = (options.h_init > 0.0) ? std::clamp(options.h_init, options.h_min, options.h_max) : choose_initial_step(t0, y, t1, dir, sys, dh, have_rhs);

        if (dh && std::isfinite(md))
        {
            h_abs = std::min(h_abs, md);
        }

        if (!(h_abs > 0.0) || !std::isfinite(h_abs))
        {
            return make_result(SolveStatus::InvalidOptions, t0, 0.0, std::numeric_limits<double>::infinity());
        }

        double t = t0;

        // ══════════════════════════════════════════════════════════════════
        // Main integration loop
        // ══════════════════════════════════════════════════════════════════

        while (dir * (t1 - t) > 0.0)
        {
            if (m_stats.steps >= options.max_steps)
            {
                return make_result(SolveStatus::MaxStepsExceeded, t, dir * h_abs, err_norm);
            }

            // ── Clip h_abs: endpoint, h_max, h_min, min_delay ────────────
            h_abs = std::clamp(std::min(h_abs, std::abs(t1 - t)), options.h_min, options.h_max);
            if (dh && std::isfinite(md))
            {
                h_abs = std::min(h_abs, md);
            }

            // ── Clip h_abs to next breaking point ────────────────────────
            if (bp_idx < bp_sched.size())
            {
                const double dist = dir * (bp_sched[bp_idx] - t);
                if (dist > 0.0 && dist < h_abs)
                {
                    h_abs = dist;
                }
            }

            const double h = dir * h_abs;

            // ── Evaluate RHS if not cached ────────────────────────────────
            if (!fsal_valid && !have_rhs)
            {
                call_rhs(t, y, sys, m_ws.k[0], dh, t);
                ++m_stats.rhs_evals;

                if (!is_finite(m_ws.k[0]))
                {
                    return make_result(SolveStatus::NonFiniteRhs, t, h, std::numeric_limits<double>::infinity());
                }

                if (dh && !dde_seeded)
                {
                    dh->set_initial_derivatives(to_std_vector(m_ws.k[0]));
                    dde_seeded = true;
                }
                have_rhs = true;
            }

            // Lambda wraps call_rhs so max_query_time = t (step start)
            auto rhs = [&](double ts, const Vec<N> &ys, Vec<N> &out) { call_rhs(ts, ys, sys, out, dh, t); };

            static_cast<Derived *>(this)->compute_step(t, y, h, rhs, m_ws, m_stats);
            ++m_stats.steps;

            if (!is_finite(m_ws.next))
            {
                return make_result(SolveStatus::NonFiniteState, t, h, std::numeric_limits<double>::infinity());
            }

            err_norm = scaled_error(y, m_ws.next, m_ws.error);
            if (!std::isfinite(err_norm))
            {
                return make_result(SolveStatus::NonFiniteError, t, h, err_norm);
            }

            const bool accepted = (err_norm <= 1.0);
            const double factor = options.controller.propose(err_norm, h_abs, adaptive_order(), ctrl, accepted);
            double next_h = std::clamp(h_abs * factor, options.h_min, options.h_max);
            if (dh && std::isfinite(md))
            {
                next_h = std::min(next_h, md);
            }

            // ── Rejected step ─────────────────────────────────────────────
            if (!accepted)
            {
                ++m_stats.rejects;
                ctrl.prev_error = std::max(err_norm, 1.0e-16);
                ctrl.has_prev_error = true;
                ctrl.previous_rejected = true;
                fsal_valid = false;
                have_rhs = false;

                const double floor = options.h_min * (1.0 + 16.0 * std::numeric_limits<double>::epsilon());
                if (h_abs <= floor && next_h <= floor)
                {
                    return make_result(SolveStatus::StepSizeUnderflow, t, h, err_norm);
                }
                h_abs = next_h;
                continue;
            }

            // ══════════════════════════════════════════════════════════════
            // Accepted step
            // ══════════════════════════════════════════════════════════════

            const double t_old = t;  // preserved for event detection

            ++m_stats.accepts;
            y = m_ws.next;
            t += h;

            ctrl.prev_error = std::max(err_norm, 1.0e-16);
            ctrl.accepted_error = std::max(err_norm, 1.0e-2);
            ctrl.accepted_h = h_abs;
            ctrl.has_prev_error = true;
            ctrl.has_accepted_reference = true;
            ctrl.previous_rejected = false;

            // ── Save endpoint to DDE history ──────────────────────────────
            if (dh)
            {
                Vec<N> ep_rhs;
                if (has_fsal())
                {
                    ep_rhs = m_ws.fsal;
                }
                else
                {
                    call_rhs(t, y, sys, ep_rhs, dh, t);
                    ++m_stats.rhs_evals;
                }
                dh->save(t, to_std_vector(y), to_std_vector(ep_rhs));
            }

            // ── FSAL: recycle k[last] as k[0] of next step ────────────────
            if (has_fsal())
            {
                m_ws.k[0] = m_ws.fsal;
                fsal_valid = true;
                have_rhs = true;
            }
            else
            {
                fsal_valid = false;
                have_rhs = false;
            }

            // ── Breaking-point crossing ───────────────────────────────────
            // Advance bp_idx past all points we just crossed; invalidate FSAL
            // because the derivative has a jump at breaking points.
            {
                bool crossed = false;
                while (bp_idx < bp_sched.size())
                {
                    const double bp = bp_sched[bp_idx];
                    const double eps = 64.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, std::abs(bp));
                    const bool past = (dir > 0.0) ? (bp <= t + eps) : (bp >= t - eps);
                    if (!past)
                    {
                        break;
                    }
                    ++bp_idx;
                    ++m_stats.breaking_points_crossed;
                    crossed = true;
                }
                if (crossed)
                {
                    fsal_valid = false;
                    have_rhs = false;
                }
            }

            // ── Periodic finiteness check ─────────────────────────────────
            if (options.integrity_check_stride > 0 && m_stats.accepts % options.integrity_check_stride == 0)
            {
                ++m_stats.integrity_checks;
                if (!is_finite(y))
                {
                    return make_result(SolveStatus::NonFiniteState, t, h, err_norm);
                }
            }

            // ── after_step hook (DoPri54 commits dense segment here) ──────
            static_cast<Derived *>(this)->after_step(t);

            // ── Event detection ───────────────────────────────────────────
            if (!options.events.empty())
            {
                if constexpr (HasLastDenseStep<Derived>::value)
                {
                    if (auto ev = detect_events(t_old, t, y, g_prev))
                    {
                        const auto [t_ev, y_ev, ei] = *ev;

                        if (options.save_history)
                        {
                            record(t_ev, h, err_norm, y_ev);
                            notify(obs, t_ev, y_ev, m_ws.error);
                        }

                        t = t_ev;
                        y = y_ev;

                        static_cast<Derived *>(this)->after_solve();
                        SolveResult res = make_result(SolveStatus::EventTriggered, t, dir * h_abs, err_norm);
                        res.event_index = ei;
                        return res;
                    }
                }
            }

            // ── Output recording ──────────────────────────────────────────
            if (uniform)
            {
                flush_uniform(obs, t1, t0, next_out, out_dt, out_idx, h, err_norm);
            }
            else
            {
                record(t, h, err_norm, y);
                notify(obs, t, y, m_ws.error);
            }

            h_abs = next_h;

        }  // end main loop

        // Final uniform-output guard
        if (uniform && out_idx < options.output_points)
        {
            record(t1, 0.0, err_norm, y);
            notify(obs, t1, y, m_ws.error);
        }

        static_cast<Derived *>(this)->after_solve();
        return make_result(SolveStatus::Success, t, dir * h_abs, err_norm);
    }
};

// Convenience alias
template <typename Derived, int N, int HistoryPoints = 5000, int MaxStages = 16>
using DES_Solver = AdaptiveDES<Derived, N, HistoryPoints, MaxStages>;

}  // namespace DES