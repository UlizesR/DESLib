#pragma once

#include <stdexcept>
#include <cmath>
#include <iostream>
#include <algorithm>

namespace DES
{

// ----------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------
template <typename Derived, typename Func>
DesBase<Derived, Func>::DesBase(std::size_t neq, bool need_jac, const Func& func)
    : name_("des")
    , method_("UnknownMethod")
    , dirout_(std::nullopt)
    , verbose_(true)    // default to printing messages
    , silent_snap_(false)
    , neq_(neq)
    , t_(0.0)
    , dt_(std::nan(""))
    , nstep_(0)
    , neval_(0)
    , icheck_(100)
    , sol_(neq, 0.0)
    , need_jac_(need_jac)
    , nJac_(0)
    , absjacdel_(1e-8)
    , reljacdel_(1e-8)
    , func_(func)
{
    if (need_jac_) {
        // Allocate space for the Jacobian
        Jac_.resize(neq_);
        for (std::size_t i = 0; i < neq_; ++i) {
            Jac_[i].resize(neq_, 0.0);
        }
        f_.resize(neq_, 0.0);
        g_.resize(neq_, 0.0);
    }
}

// -------------------------
// set_sol with std::move
// -------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::set_sol(std::vector<double> s) {
    if (s.size() != neq_) {
        throw std::runtime_error("set_sol: input size mismatch");
    }
    // (#10) Move the vector instead of copying
    sol_ = std::move(s);
}

// ----------------------------------------------------------------
// Step: calls Derived::step_impl(dt)
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::step(double dt, bool extra) {
    dt_ = dt;
    derived().step_impl(dt);   // CRTP call
    t_ += dt;
    ++nstep_;

    if (nstep_ % icheck_ == 0) {
        check_sol_integrity();
    }
    if (extra) {
        after_step(t_);
    }
}

// ----------------------------------------------------------------
// Solve (fixed-step) - no output
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::solve_fixed(double tint, double dt, bool extras) {
    check_pre_solve(tint, dt);
    double tend = t_ + tint;

    if (extras) {
        before_solve();
    }
    // Solve until within dt of 'tend'
    while (!solve_done(dt, tend)) {
        step(dt, extras);
    }
    // Final partial step
    double final_dt = tend - t_;
    if (final_dt < 1e-15) {
        // (#9) If the final dt is extremely small, throw an error (or do something else)
        throw std::runtime_error("Time step underflow in solve_fixed (final_dt < 1e-15).");
    }
    step(final_dt, extras);

    if (extras) {
        after_solve();
    }
}

// ----------------------------------------------------------------
// Solve (fixed-step) with output at given interval
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::solve_fixed(double tint, double dt, const std::string& dirout, int inter) {
    check_pre_solve(tint, dt);
    if (inter < 1) {
        throw std::runtime_error("solve_fixed: inter must be >= 1");
    }

    // (#13) store directory in optional
    set_dirout(dirout);

    before_solve();

    double tend = t_ + tint;

    // (#8) Reserve space for time and solutions to reduce re-allocations
    std::size_t estimate = static_cast<std::size_t>(std::ceil((tend - t_) / dt)) + 2;
    std::vector<double> tout;
    tout.reserve(estimate);

    std::vector<std::vector<double>> solout(neq_);
    for (auto& v : solout) {
        v.reserve(estimate);
    }

    // Store initial
    for (std::size_t i = 0; i < neq_; ++i) {
        solout[i].push_back(sol_[i]);
    }
    tout.push_back(t_);

    std::size_t j = 0;
    while (!solve_done(dt, tend)) {
        step(dt);
        if (j % static_cast<std::size_t>(inter) == 0) {
            after_capture(t_);
            for (std::size_t i = 0; i < neq_; ++i) {
                solout[i].push_back(sol_[i]);
            }
            tout.push_back(t_);
        }
        ++j;
    }
    // Final partial step
    double final_dt = tend - t_;
    if (final_dt < 1e-15) {
        throw std::runtime_error("Time step underflow (final_dt < 1e-15).");
    }
    step(final_dt);
    for (std::size_t i = 0; i < neq_; ++i) {
        solout[i].push_back(sol_[i]);
    }
    tout.push_back(t_);

    // If dirout_ is set, do output
    if (dirout_) {
        for (std::size_t i = 0; i < neq_; ++i) {
            std::string fn = *dirout_ + "/" + name_ + "_" + des_int_to_string(i);
            des_write(fn, solout[i]);
        }
        {
            std::string fn = *dirout_ + "/" + name_ + "_t";
            des_write(fn, tout);
        }
    }

    after_solve();
    clear_dirout();
}

// ----------------------------------------------------------------
// Solve (fixed-step) with evenly spaced snapshots
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::solve_fixed(double tint, double dt, std::size_t nsnap, const std::string& dirout) {
    check_pre_solve(tint, dt);

    double tend = t_ + tint;
    std::vector<double> tsnap(nsnap, 0.0);
    for (std::size_t i = 0; i < nsnap; ++i) {
        tsnap[i] = t_ + i * (tend - t_) / double(nsnap - 1);
    }
    solve_fixed(dt, tsnap, dirout);
}

// ----------------------------------------------------------------
// Solve (fixed-step) with user-specified snapshot times
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::solve_fixed(double dt, const std::vector<double>& tsnap, const std::string& dirout) {
    check_pre_snaps(dt, tsnap);

    set_dirout(dirout);
    before_solve();

    // solve to each snap time
    for (std::size_t i = 0; i < tsnap.size(); ++i) {
        double segment = tsnap[i] - t_;
        if (segment < 1e-15) {
            // (#9) check small dt
            throw std::runtime_error("solve_fixed: segment < 1e-15 => underflow or times not strictly increasing?");
        }
        solve_fixed_(segment, dt);
        snap(*dirout_, i, t_);
    }
    // Write snap times if not silent
    if (!silent_snap_ && dirout_) {
        std::string fn = *dirout_ + "/" + name_ + "_snap_t";
        des_write(fn, tsnap);
    }

    after_solve();
    clear_dirout();
}

// ----------------------------------------------------------------
// Reset
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::reset(double t, const std::vector<double>& sol) {
    if (sol.size() != neq_) {
        throw std::runtime_error("reset: solution size mismatch");
    }
    t_ = t;
    sol_ = sol;
}

// ----------------------------------------------------------------
// Evaluate ODE system f
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::des_fun_(const std::vector<double>& solin,
                                      std::vector<double>& fout)
{
    // CRTP call to derived method:
    derived().des_fun(solin, fout);

    ++neval_;
}

// ----------------------------------------------------------------
// Evaluate ODE Jacobian (analytical or numeric fallback)
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::des_jac_(const std::vector<double>& solin,
                                      std::vector<std::vector<double>>& Jout)
{
    ++nJac_;
    if (!need_jac_) {
        throw std::runtime_error("Jacobian not allocated (need_jac_ == false)");
    }

    // Default: numeric approximation
    numerical_jac(solin, Jout);
}

// ----------------------------------------------------------------
// Numeric approximation of Jacobian
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::numerical_jac(const std::vector<double>& solin,
                                           std::vector<std::vector<double>>& Jout)
{
    des_fun_(solin, f_);

    for (std::size_t i = 0; i < neq_; ++i) {
        double delsol = des_max2(absjacdel_, solin[i] * reljacdel_);

        // Perturb
        std::vector<double> sol_pert = solin;
        sol_pert[i] += delsol;

        des_fun_(sol_pert, g_);

        // Partial derivatives
        for (std::size_t j = 0; j < neq_; ++j) {
            Jout[j][i] = (g_[j] - f_[j]) / delsol;
        }
    }
}

// ----------------------------------------------------------------
// Internal partial solver
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::solve_fixed_(double tint, double dt, bool extra) {
    double tend = t_ + tint;
    while (!solve_done(dt, tend)) {
        step(dt, extra);
    }
    double final_dt = tend - t_;
    if (final_dt < 1e-15) {
        throw std::runtime_error("Time step underflow in solve_fixed_ (final_dt < 1e-15).");
    }
    step(final_dt, extra);
}

// ----------------------------------------------------------------
// Snap support
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::snap(const std::string& dirout,
                                  std::size_t isnap,
                                  double tsnap)
{
    if (silent_snap_) {
        if (verbose_) {
            std::cout << "[snap] " << isnap << " reached @t=" << tsnap << "\n";
        }
        after_snap(dirout, isnap, tsnap);
    } else {
        // Write file
        std::string fn = dirout + "/" + name_ + "_snap_" + des_int_to_string(isnap);
        des_write(fn, sol_);
        if (verbose_) {
            std::cout << "[snap] " << isnap << " written @t=" << tsnap << "\n";
        }
        after_snap(dirout, isnap, tsnap);
    }
}

// ----------------------------------------------------------------
// Solve done check
// ----------------------------------------------------------------
template <typename Derived, typename Func>
bool DesBase<Derived, Func>::solve_done(double dt, double tend) {
    // If the next step would exceed the end by 1% => partial step
    return ((t_ + dt * 1.01) >= tend);
}

// ----------------------------------------------------------------
// Integrity checks
// ----------------------------------------------------------------
template <typename Derived, typename Func>
void DesBase<Derived, Func>::check_sol_integrity() {
    for (std::size_t i = 0; i < neq_; ++i) {
        if (std::isnan(sol_[i])) {
            throw std::runtime_error("check_sol_integrity: solution contains NaN");
        }
        if (!std::isfinite(sol_[i])) {
            throw std::runtime_error("check_sol_integrity: solution contains Inf");
        }
    }
}

template <typename Derived, typename Func>
void DesBase<Derived, Func>::check_pre_solve(double tint, double dt) {
    if (tint <= 0.0) {
        throw std::runtime_error("check_pre_solve: tint must be > 0");
    }
    if (dt <= 0.0) {
        throw std::runtime_error("check_pre_solve: dt must be > 0");
    }
}

template <typename Derived, typename Func>
void DesBase<Derived, Func>::check_pre_snaps(double dt, const std::vector<double>& tsnap) {
    if (dt <= 0.0) {
        throw std::runtime_error("check_pre_snaps: dt must be > 0");
    }
    if (tsnap.size() <= 1) {
        throw std::runtime_error("check_pre_snaps: nsnap must be > 1");
    }
    for (std::size_t i = 0; i < tsnap.size(); ++i) {
        if (tsnap[i] < t_) {
            throw std::runtime_error("check_pre_snaps: snapshot times must be >= current time");
        }
        if (i > 0 && tsnap[i] <= tsnap[i - 1]) {
            throw std::runtime_error("check_pre_snaps: snapshot times must be strictly increasing");
        }
    }
}

} // namespace DES

