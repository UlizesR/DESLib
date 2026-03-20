#pragma once

// DES — Differential Equation Solver.  Eigen backend, C++17.
// Requires Eigen 3.3+.

#include <Eigen/Dense>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace DES {

// ---------------------------------------------------------------------------
// Primary vector type: Eigen fixed-size column vector
// ---------------------------------------------------------------------------

template <int N>
using Vec = Eigen::Matrix<double, N, 1>;

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

enum class ErrorNorm {
    Rms,
    Infinity
};
enum class ControllerKind {
    Integral,
    PI,
    Gustafsson
};

enum class SolveStatus {
    // ── Normal termination ────────────────────────────────────────────────
    Success,
    EventTriggered,  // a terminal event function fired

    // ── Configuration errors ──────────────────────────────────────────────
    InvalidOptions,

    // ── Integration failures ──────────────────────────────────────────────
    StepSizeUnderflow,
    MaxStepsExceeded,
    NonFiniteRhs,
    NonFiniteError,
    NonFiniteState,

    // ── DDE-specific failures ─────────────────────────────────────────────
    TerminationDetected,   // state-dependent delay: D⁺α<0 && D⁻α>0
    BreakingPointFailure,  // bisection for breaking-point location failed
};

// ---------------------------------------------------------------------------
// Workspace — scratch arrays for Runge–Kutta stage evaluations
// ---------------------------------------------------------------------------

template <int N, int MaxStages = 16>
struct Workspace {
    std::array<Vec<N>, MaxStages> k{};
    Vec<N> next{};
    Vec<N> error{};
    Vec<N> fsal{};  // first-same-as-last endpoint, reused as k[0] next step
};

// ---------------------------------------------------------------------------
// SolverStats — accumulated counters for a single solve() call
// ---------------------------------------------------------------------------

struct SolverStats {
    long steps = 0;
    long accepts = 0;
    long rejects = 0;
    long rhs_evals = 0;
    long integrity_checks = 0;
    long history_writes = 0;

    long breaking_points_crossed = 0;  // enforced mesh points crossed
    long events_triggered = 0;         // sign-changes detected (all events)
    long bisection_iters = 0;          // total bisection iterations
};

// ---------------------------------------------------------------------------
// SolveResult — returned from solver::solve()
// ---------------------------------------------------------------------------

struct SolveResult {
    SolveStatus status = SolveStatus::Success;
    double t_final = 0.0;
    double last_h = 0.0;
    double last_error_norm = 0.0;
    long history_size = 0;

    int event_index = -1;  // index of fired event; -1 if none
    long breaking_points_crossed = 0;

    [[nodiscard]] bool ok() const noexcept
    {
        return status == SolveStatus::Success || status == SolveStatus::EventTriggered;
    }
};

// ---------------------------------------------------------------------------
// ControllerState — persistent data for multi-step error controllers
// ---------------------------------------------------------------------------

struct ControllerState {
    double prev_error = 1.0;
    double accepted_error = 1.0;
    double accepted_h = 0.0;
    bool has_prev_error = false;
    bool has_accepted_reference = false;
    bool previous_rejected = false;
};

// ---------------------------------------------------------------------------
// StepController
// ---------------------------------------------------------------------------

struct StepController {
    ControllerKind kind = ControllerKind::PI;
    double safety = 0.9;
    double min_factor = 0.2;
    double max_factor = 10.0;
    double steady_min = 0.98;
    double steady_max = 1.02;

    // PI exponents; negative → derive from method order
    double alpha = -1.0;
    double beta = -1.0;

    [[nodiscard]] double propose(double error_norm, double h_abs, int adaptive_order, const ControllerState &state, bool accepted) const noexcept
    {
        if (adaptive_order < 0 || !std::isfinite(error_norm))
        {
            return min_factor;
        }
        if (error_norm <= 0.0)
        {
            return max_factor;
        }

        const double inv_q = 1.0 / static_cast<double>(adaptive_order + 1);
        double factor = safety * std::pow(error_norm, -inv_q);

        if (kind == ControllerKind::PI && state.has_prev_error)
        {
            const double a = (alpha >= 0.0) ? alpha : 0.7 * inv_q;
            const double b = (beta >= 0.0) ? beta : 0.4 * inv_q;
            factor = safety * std::pow(error_norm, -a) * std::pow(std::max(state.prev_error, 1.0e-16), b);
        }
        else if (kind == ControllerKind::Gustafsson && accepted && !state.previous_rejected && state.has_accepted_reference && state.accepted_h > 0.0)
        {
            const double gust = safety * (h_abs / state.accepted_h) * std::pow(std::max(state.accepted_error, 1.0e-16) / std::max(error_norm * error_norm, 1.0e-32), inv_q);
            factor = std::min(factor, gust);
        }

        if (!accepted || state.previous_rejected)
        {
            factor = std::min(factor, 1.0);
        }
        factor = std::clamp(factor, min_factor, max_factor);
        if (factor >= steady_min && factor <= steady_max)
        {
            factor = 1.0;
        }
        return factor;
    }
};

// ---------------------------------------------------------------------------
// Compile-time capability detection (CRTP trait detectors)
// ---------------------------------------------------------------------------

template <typename T, typename = void>
struct HasMethodOrder : std::false_type {};
template <typename T>
struct HasMethodOrder<T, std::void_t<decltype(T::method_order())>> : std::true_type {};

template <typename T, typename = void>
struct HasAdaptiveOrder : std::false_type {};
template <typename T>
struct HasAdaptiveOrder<T, std::void_t<decltype(T::adaptive_order())>> : std::true_type {};

template <typename T, typename = void>
struct HasFsal : std::false_type {};
template <typename T>
struct HasFsal<T, std::void_t<decltype(T::has_fsal())>> : std::true_type {};

template <typename T, typename = void>
struct HasLastDenseStep : std::false_type {};
template <typename T>
struct HasLastDenseStep<T, std::void_t<decltype(std::declval<const T &>().last_dense_step())>> : std::true_type {};

}  // namespace DES