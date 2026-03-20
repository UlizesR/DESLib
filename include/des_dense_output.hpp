#pragma once

/*  des_dense_output.hpp  –  DES namespace
 *
 *  Shared dense-output infrastructure.
 *
 *  DenseSegment<N>    — polynomial evaluated via Horner's method
 *  hermite_segment<N> — Hermite-cubic segment from endpoint (y,f) pairs
 *
 *  All DES solvers that expose last_dense_step() include this header.
 *  C++17.  Requires DES.hpp (Eigen).
 */

#include "DES.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace DES {

// ---------------------------------------------------------------------------
// DenseSegment<N>
//
// Stores one accepted step's dense-output polynomial in Horner form:
//
//   y(t₀ + θ·h) = y₀ + h·θ·(q[0] + θ·(q[1] + θ·(q[2] + θ·q[3])))
//   θ ∈ [0, 1]
//
// Coefficient vectors q[0..3] are solver-specific:
//   DoPri54   – 4th-order continuous extension (7 stage evals)
//   DoPri87   – Hermite cubic, O(h⁴)
//   Rosenbrock4 – Hermite cubic, O(h⁴)
// ---------------------------------------------------------------------------

template <int N>
struct DenseSegment {
    double t0 = 0.0;
    double h = 0.0;
    bool valid = false;
    Vec<N> y0{};
    std::array<Vec<N>, 4> q{};

    // True iff t ∈ [t0, t0+h] in the direction of h, with ε tolerance
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

    // Evaluate at θ ∈ [0,1]: 8 multiplications via Horner's method
    [[nodiscard]] Vec<N> eval_theta(double theta) const
    {
        if (!valid)
        {
            throw std::logic_error("DenseSegment: dense output not available");
        }
        theta = std::clamp(theta, 0.0, 1.0);
        return y0 + (h * theta) * (q[0] + theta * (q[1] + theta * (q[2] + theta * q[3])));
    }

    // Evaluate at absolute time t ∈ [t0, t0+h]
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
// hermite_segment<N>
//
// Builds a DenseSegment from the Hermite cubic determined by:
//   (t0,      y0, f0)  — state and derivative at step start
//   (t0 + h,  y1, f1)  — state and derivative at step end
//
// The unique cubic p(θ) satisfying the four conditions is:
//   p(0) = y₀,  p'(0) = f₀·h,  p(1) = y₁,  p'(1) = f₁·h
//
// In Horner form y₀ + h·θ·(q₀ + θ(q₁ + θ q₂)):
//   q₀ = f₀
//   q₁ = 3(y₁−y₀)/h − 2f₀ − f₁
//   q₂ = −2(y₁−y₀)/h + f₀ + f₁
//   q₃ = 0
//
// Interpolation error O(h⁴).
// ---------------------------------------------------------------------------

template <int N>
[[nodiscard]] DenseSegment<N> hermite_segment(double t0, const Vec<N> &y0, double h, const Vec<N> &y1, const Vec<N> &f0, const Vec<N> &f1) noexcept
{
    DenseSegment<N> seg;
    seg.t0 = t0;
    seg.h = h;
    seg.y0 = y0;
    seg.valid = true;

    const Vec<N> slope = (y1 - y0) * (1.0 / h);  // (y₁−y₀) / h

    seg.q[0] = f0;
    seg.q[1] = 3.0 * slope - 2.0 * f0 - f1;
    seg.q[2] = -2.0 * slope + f0 + f1;
    seg.q[3].setZero();  // cubic: q[3] = 0

    return seg;
}

}  // namespace DES