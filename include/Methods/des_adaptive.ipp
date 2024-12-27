#pragma once

#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace DES
{

//------------------------------------------------
// Constructor
//------------------------------------------------
template <typename Derived, typename Func>
DesAdaptiveBase<Derived, Func>::DesAdaptiveBase(std::size_t neq, bool need_jac, const Func& func)
    : DesBase<Derived, Func>(neq, need_jac, func)
    , n_reject_(0)
    , abstol_(1e-6)
    , reltol_(1e-6)
    , dtmax_(std::numeric_limits<double>::infinity())
    , sol_prev_(neq, 0.0)
{
    this->set_method("AdaptiveBase"); // Optional method name
}

//------------------------------------------------
// solve_adaptive_impl (core loop, no I/O output)
//------------------------------------------------
template <typename Derived, typename Func>
void DesAdaptiveBase<Derived, Func>::solve_adaptive_impl(double tint, double dt0, bool extra)
{
    double dt = dt0;
    double tend = this->t_ + tint;

    // If dt overshoots end, shorten it
    if (this->t_ + dt > tend) {
        dt = tend - this->t_;
    }
    // Keep stepping until done
    while (!solve_done_adaptive(tend)) {
        // Attempt one step
        bool accepted = step_adaptive_impl(dt, extra);
        // Get next dt from derived adaptation (or numeric fallback)
        dt = dt_adapt_clipped(tend);

        // (#9) dt underflow check
        if (dt < 1e-15) {
            throw std::runtime_error("DesAdaptiveBase: dt underflow (< 1e-15).");
        }
        // If step was accepted, we proceed in time. If rejected, step_adaptive_impl reverts.
    }
}

//------------------------------------------------
// Public solve_adaptive variants
//------------------------------------------------
template <typename Derived, typename Func>
void DesAdaptiveBase<Derived, Func>::solve_adaptive(double tint, double dt0, bool extras)
{
    this->check_pre_solve(tint, dt0);

    if (extras) {
        this->before_solve();
        solve_adaptive_impl(tint, dt0, true);
        this->after_solve();
    } else {
        solve_adaptive_impl(tint, dt0, false);
    }
}

template <typename Derived, typename Func>
void DesAdaptiveBase<Derived, Func>::solve_adaptive(double tint,
                                                    double dt0,
                                                    const std::string& dirout,
                                                    int inter)
{
    if (inter < 1) {
        throw std::runtime_error("solve_adaptive: inter must be >= 1");
    }
    this->check_pre_solve(tint, dt0);

    // (#13) store output dir in optional
    this->set_dirout(dirout);
    this->before_solve();

    double tend = this->t_ + tint;
    std::size_t j = 0;

    // Reserve space for times and solutions if you want to store them
    std::vector<double> tout;
    tout.push_back(this->t_);
    std::vector<std::vector<double>> solout(this->neq_);
    for (std::size_t i = 0; i < this->neq_; ++i) {
        solout[i].push_back(this->sol_[i]);
    }

    double dt = dt0;
    while (!solve_done_adaptive(tend)) {
        bool accepted = step_adaptive_impl(dt);
        dt = dt_adapt_clipped(tend);

        if (accepted) {
            j++;
            if (j % static_cast<std::size_t>(inter) == 0) {
                this->after_capture(this->t_);
                for (std::size_t i = 0; i < this->neq_; ++i) {
                    solout[i].push_back(this->sol_[i]);
                }
                tout.push_back(this->t_);
            }
        }

        if (dt < 1e-15) {
            throw std::runtime_error("solve_adaptive: dt underflow (< 1e-15).");
        }
    }
    // Optionally store final point if needed
    // ...

    // Write to disk if dirout_ is set
    if (this->get_dirout()) {
        for (std::size_t i = 0; i < this->neq_; ++i) {
            std::string fn = *(this->get_dirout()) + "/" + this->name_ + "_" + des_int_to_string(i);
            des_write(fn, solout[i]);
        }
        {
            std::string fn = *(this->get_dirout()) + "/" + this->name_ + "_t";
            des_write(fn, tout);
        }
    }

    this->after_solve();
    this->clear_dirout();
}

template <typename Derived, typename Func>
void DesAdaptiveBase<Derived, Func>::solve_adaptive(double tint,
                                                    double dt0,
                                                    std::size_t nsnap,
                                                    const std::string& dirout)
{
    this->check_pre_solve(tint, dt0);

    double tend = this->t_ + tint;
    std::vector<double> tsnap(nsnap);
    for (std::size_t i = 0; i < nsnap; ++i) {
        tsnap[i] = this->t_ + i * (tend - this->t_) / double(nsnap - 1);
    }
    solve_adaptive(dt0, tsnap, dirout);
}

template <typename Derived, typename Func>
void DesAdaptiveBase<Derived, Func>::solve_adaptive(double dt0,
                                                    const std::vector<double>& tsnap,
                                                    const std::string& dirout)
{
    this->check_pre_snaps(dt0, tsnap);

    this->set_dirout(dirout);
    this->before_solve();

    double dt = dt0;
    for (std::size_t i = 0; i < tsnap.size(); ++i) {
        double segment = tsnap[i] - this->t_;
        if (segment < 1e-15) {
            throw std::runtime_error("solve_adaptive: snapshot segment < 1e-15 => times not strictly increasing?");
        }
        // integrate up to tsnap[i]
        solve_adaptive_impl(segment, dt);
        // snap
        if (this->get_dirout()) {
            this->snap(*(this->get_dirout()), i, this->t_);
        }
        // record the new dt
        dt = dt_adapt_clipped(std::numeric_limits<double>::infinity());
        if (dt < 1e-15) {
            throw std::runtime_error("solve_adaptive: dt underflow (< 1e-15).");
        }
    }

    // write snap times, if not silent
    if (!this->silent_snap_ && this->get_dirout()) {
        std::string fn = *(this->get_dirout()) + "/" + this->name_ + "_snap_t";
        des_write(fn, tsnap);
    }

    this->after_solve();
    this->clear_dirout();
}

//------------------------------------------------
// step_adaptive_impl
//------------------------------------------------
template <typename Derived, typename Func>
bool DesAdaptiveBase<Derived, Func>::step_adaptive_impl(double dt, bool extra)
{
    // Save current solution in case we must revert
    sol_prev_ = this->sol_;

    // (this->dt_ is updated so derived can see it, if needed)
    this->dt_ = dt;
    // CRTP call to the underlying stepping method
    this->derived().step_impl(dt);

    // Increase step counter
    ++this->nstep_;

    // Let derived do error estimate, etc.
    this->derived().adapt(abstol_, reltol_);

    // Check if step is to be rejected
    if (this->derived().is_rejected()) {
        // revert solution
        this->sol_ = sol_prev_;
        // increment reject count
        ++n_reject_;
        // Return false
        return false;
    } else {
        // step accepted => advance time
        this->t_ += dt;
        // optional integrity check
        if (this->nstep_ % this->icheck_ == 0) {
            this->check_sol_integrity();
        }
        if (extra) {
            this->after_step(this->t_);
        }
        return true;
    }
}

//------------------------------------------------
// dt_adapt_clipped
//------------------------------------------------
template <typename Derived, typename Func>
double DesAdaptiveBase<Derived, Func>::dt_adapt_clipped(double tend)
{
    // Let derived pick next dt
    double dt = this->derived().dt_adapt();

    // Clip so we don't exceed the endpoint
    if (tend < this->t_ + dt * 1.01) {
        dt = tend - this->t_;
    }
    // Clip so we don't exceed dtmax_
    if (dt > dtmax_) {
        dt = dtmax_;
    }
    return dt;
}

//------------------------------------------------
// solve_done_adaptive
//------------------------------------------------
template <typename Derived, typename Func>
bool DesAdaptiveBase<Derived, Func>::solve_done_adaptive(double tend)
{
    // If we are extremely close or beyond
    if (des_is_close(this->t_, tend, 1e-13) || (this->t_ >= tend)) {
        return true;
    }
    return false;
}

} // namespace DES

