# DESLib

A C++ library for solving ordinary differential equations (ODEs) and delay differential equations (DDEs).

DESLib grew out of undergraduate research completed with Professor John Stratton at Whitman College. The current codebase focuses on adaptive one-step methods with dense output, practical support for retarded delay equations, and a small set of example problems for non-stiff and stiff systems.

## Current status

The library currently includes:

- `DES::DoPri54<N>` — Dormand–Prince 5(4), a good default choice for many non-stiff ODEs and DDEs.
- `DES::DoPri87<N>` — Dormand–Prince 8(7), useful when a higher-order explicit method is worth the extra stage cost.
- `DES::Rosenbrock4<N>` — a 4-stage GRK4A Rosenbrock method for stiff systems, with either an analytical Jacobian or a finite-difference fallback.

The adaptive base solver also provides:

- adaptive step-size control (`Integral`, `PI`, or `Gustafsson` controllers)
- dense output / interpolation between accepted steps
- event detection by zero-crossing
- saved solution history for plotting or post-processing
- support for retarded DDEs through a causality-checked history view
- breaking-point handling for declared constant delays

## Requirements

- C++17
- Eigen 3.3 or newer

DESLib is currently organized as a header-based library. In the uploaded headers, the core vector type is `DES::Vec<N>`, which aliases a fixed-size `Eigen::Matrix<double, N, 1>`.

## Repository layout


```cpp
#include "../include/Methods/des_dopri54.hpp"
```

Adjust the include path to match your local tree.

## Quick start

### 1. Non-stiff ODE example

```cpp
#include "../include/Methods/des_dopri54.hpp"

struct LorenzSystem {
    double sigma = 10.0;
    double rho = 28.0;
    double beta = 8.0 / 3.0;

    void operator()(double /*t*/, const DES::Vec<3>& y, DES::Vec<3>& dydt) const {
        dydt[0] = sigma * (y[1] - y[0]);
        dydt[1] = y[0] * (rho - y[2]) - y[1];
        dydt[2] = y[0] * y[1] - beta * y[2];
    }
};

int main() {
    using Solver = DES::DoPri54<3>;
    using State  = DES::Vec<3>;

    Solver solver;
    solver.options.rtol = 1.0e-9;
    solver.options.atol = 1.0e-12;
    solver.options.h_init = 1.0e-3;
    solver.options.h_max = 0.02;
    solver.options.save_history = true;
    solver.options.uniform_output = true;
    solver.options.output_points = 10000;

    State y;
    y << 1.0, 1.0, 1.0;

    LorenzSystem rhs;
    const auto result = solver.solve(y, 0.0, 100.0, rhs);

    if (!result.ok()) {
        return 1;
    }

    const auto& hist = solver.history();
    // hist.t, hist.y, hist.error, hist.h
}
```

### 2. Delayed DDE example

```cpp
#include <functional>
#include <vector>
#include "../include/Methods/des_dopri54.hpp"
#include "history.hpp"

struct MackeyGlass {
    double beta  = 0.2;
    double gamma = 0.1;
    double n     = 10.0;
    double tau   = 2.0;

    void operator()(double t,
                    const DES::Vec<1>& y,
                    const DES::DelayHistoryView<1>& hist,
                    DES::Vec<1>& dydt) const {
        const double x_tau = hist(0, t - tau);
        dydt[0] = beta * x_tau / (1.0 + std::pow(x_tau, n)) - gamma * y[0];
    }
};

int main() {
    using Solver = DES::DoPri54<1>;
    using State  = DES::Vec<1>;

    MackeyGlass rhs;
    Solver solver;

    solver.options.rtol = 1.0e-8;
    solver.options.atol = 1.0e-10;
    solver.options.h_init = 1.0e-3;
    solver.options.h_max = 0.1;
    solver.options.min_delay = rhs.tau;
    solver.options.save_history = true;
    solver.options.uniform_output = true;
    solver.options.output_points = 1000;

    State y;
    y[0] = 1.2;

    const std::vector<double> max_delays = {rhs.tau};
    const std::vector<double> init_conds = {1.2};
    const std::vector<std::function<double(double)>> prehistory = {
        [](double /*t*/) { return 1.2; }
    };

    DES::History<double, double> dde_hist(
        1, 0.0, 0.0, max_delays, init_conds, prehistory);

    const auto result = solver.solve(y, 0.0, 50.0, rhs, dde_hist);
    if (!result.ok()) {
        return 1;
    }
}
```

### 3. Stiff ODE example

```cpp
#include "../include/Methods/des_rossenbrock.hpp"

struct RobertsonSystem {
    void operator()(double /*t*/, const DES::Vec<3>& y, DES::Vec<3>& dydt) const {
        const double y1 = y[0];
        const double y2 = y[1];
        const double y3 = y[2];

        dydt[0] = -0.04 * y1 + 1.0e4 * y2 * y3;
        dydt[1] =  0.04 * y1 - 1.0e4 * y2 * y3 - 3.0e7 * y2 * y2;
        dydt[2] =  3.0e7 * y2 * y2;
    }

    void jacobian(double /*t*/, const DES::Vec<3>& y,
                  Eigen::Matrix<double, 3, 3>& J) const {
        const double y2 = y[1];
        const double y3 = y[2];

        J(0, 0) = -0.04;
        J(0, 1) =  1.0e4 * y3;
        J(0, 2) =  1.0e4 * y2;

        J(1, 0) =  0.04;
        J(1, 1) = -1.0e4 * y3 - 6.0e7 * y2;
        J(1, 2) = -1.0e4 * y2;

        J(2, 0) =  0.0;
        J(2, 1) =  6.0e7 * y2;
        J(2, 2) =  0.0;
    }
};

int main() {
    using Solver = DES::Rosenbrock4<3>;
    using State  = DES::Vec<3>;

    Solver solver;
    solver.options.rtol = 1.0e-7;
    solver.options.atol = 1.0e-12;
    solver.options.h_init = 1.0e-6;
    solver.options.h_max = 100.0;

    State y;
    y << 1.0, 0.0, 0.0;

    RobertsonSystem rhs;
    const auto result = solver.solve(y, 0.0, 100.0, rhs);
    if (!result.ok()) {
        return 1;
    }
}
```

## Solver options you will use most often

The adaptive solvers expose an `options` struct. The settings you will likely touch first are:

- `rtol`, `atol` — scalar tolerances
- `atol_vec`, `use_vector_atol` — per-component absolute tolerances
- `h_init`, `h_min`, `h_max` — step-size controls
- `max_steps` — safety cap
- `save_history` — store accepted output
- `uniform_output` and `output_points` — resample to a uniform output grid when dense output is available
- `min_delay` — declare a smallest delay so DDE history queries remain outside the current step
- `controller.kind` — choose `Integral`, `PI`, or `Gustafsson`
- `declared_delays` / `detect_breaking_points` — enforce known breaking points for constant-delay problems
- `events` — terminal or non-terminal zero-crossing events

Saved history can be accessed through:

```cpp
const auto& hist = solver.history();
// hist.t      : times
// hist.y      : states
// hist.error  : error estimate norms
// hist.h      : accepted step sizes / output step metadata
```

## Choosing a solver

- Use **DoPri54** as the default explicit method for many smooth, non-stiff ODEs and retarded DDEs.
- Use **DoPri87** when you want a higher-order explicit method and the extra work per step is justified.
- Use **Rosenbrock4** for stiff systems, especially when you can provide an analytical Jacobian.

## Examples

The current examples include:

- a Lorenz ODE example written with `DES::DoPri54<3>`
- a circadian-clock DDE example written with `DES::DoPri54<1>` and `DES::DelayHistoryView<1>`
- a stiff Robertson kinetics example written with `DES::Rosenbrock4<3>`

Each example writes a CSV file with time, state values, local error information, and step-size metadata so the results can be plotted afterward.

## References

### Core numerical ODE references

1. J. R. Dormand and P. J. Prince, *A family of embedded Runge–Kutta formulae*, Journal of Computational and Applied Mathematics, 6(1), 19–26, 1980.
2. P. J. Prince and J. R. Dormand, *High order embedded Runge–Kutta formulae*, Journal of Computational and Applied Mathematics, 7, 67–75, 1981.
3. P. Kaps and P. Rentrop, *Generalized Runge–Kutta methods of order four with stepsize control for stiff ordinary differential equations*, Numerische Mathematik, 33, 55–68, 1979.
4. E. Hairer, S. P. Nørsett, and G. Wanner, *Solving Ordinary Differential Equations I: Nonstiff Problems*, 2nd ed., Springer, 1993.
5. E. Hairer and G. Wanner, *Solving Ordinary Differential Equations II: Stiff and Differential-Algebraic Problems*, 2nd ed., Springer, 1996.
6. J. C. Butcher, *Numerical Methods for Ordinary Differential Equations*, 2nd ed., Wiley, 2008.

### Delay differential equation references

7. L. F. Shampine and S. Thompson, *Solving DDEs in MATLAB*, Applied Numerical Mathematics, 37, 441–458, 2001.
8. N. Guglielmi and E. Hairer, *Solving delay differential equations*, chapter on dense output, breaking points, and practical DDE software.
9. O. Arino, M. L. Hbid, and E. Ait Dads (eds.), *Delay Differential Equations and Applications*, Springer, 2006.
10. S. Ruan, *Delay Differential Equations in Single Species Dynamics*, in *Delay Differential Equations and Applications*, Springer, 2006.

## Possible future methods and features

A reasonable roadmap for DESLib would be:

- **Radau IIA / collocation methods** for stiff ODEs and stiff DDEs
- **BDF or Nordsieck-style multistep methods** for large stiff systems
- **state-dependent delay support with stronger breaking-point handling**
- **neutral and distributed delay equations**
- **sparse / banded Jacobian support** and faster linear solves for large systems
- **better event handling for delayed systems**, including events defined on delayed quantities
- **benchmark and convergence test suites** against standard ODE/DDE problems
- **more examples** from population dynamics, epidemiology, and chemical kinetics
- **improved plotting / analysis utilities**
- **Python bindings or a small C API** for easier experimentation
- **parameter continuation / bifurcation tooling** for DDE test problems
- **sensitivity analysis / variational equations / adjoint support**

## Acknowledgments

DESLib is based on undergraduate research completed with Professor John Stratton while at Whitman College.
