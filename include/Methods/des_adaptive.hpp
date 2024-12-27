#pragma once

#include <vector>
#include <cmath>
#include <string>
#include <limits>  
#include <optional>

#include "des_base.hpp"
#include "../utils/des_io.hpp"
#include "../utils/des_util.hpp"

namespace DES
{

/**
 * @brief CRTP base class implementing solver functions with adaptive time steps.
 *
 * @tparam Derived The derived adaptive solver
 * @tparam Func    The function or functor describing your ODE system
 *
 * Inherits from DesBase<Derived, Func>. Expects the derived class to implement:
 * - `void adapt(double abstol, double reltol)` for adjusting step size, error estimates, etc.
 * - `bool is_rejected()` returning whether the previous step must be rolled back.
 * - `double dt_adapt()` returning the next step size (whether or not the previous step was rejected).
 */
template <typename Derived, typename Func>
class DesAdaptiveBase : public DesBase<Derived, Func>
{
public:
    /**
     * @brief Construct an adaptive solver.
     * @param neq       Number of equations
     * @param need_jac  Whether a Jacobian is needed
     * @param func      The ODE system functor/object
     */
    DesAdaptiveBase(std::size_t neq, bool need_jac, const Func& func);

    /**
     * @brief Destructor
     */
    ~DesAdaptiveBase() = default;

    // ----------------
    // Getters / Setters
    // ----------------
    inline std::size_t get_nrej()     const { return n_reject_; }
    inline double      get_abstol()   const { return abstol_;   }
    inline double      get_reltol()   const { return reltol_;   }
    inline double      get_dtmax()    const { return dtmax_;    }

    inline void set_abstol(double tol) { abstol_ = tol; }
    inline void set_reltol(double tol) { reltol_ = tol; }
    /**
     * @brief Sets both absolute and relative tolerance
     */
    inline void set_tol(double tol) {
        abstol_ = tol;
        reltol_ = tol;
    }
    inline void set_dtmax(double dtmax) { dtmax_ = dtmax; }

    // ---------------------------------------
    // Adaptive integration routines
    // ---------------------------------------
    void solve_adaptive(double tint, double dt0, bool extras = true);
    void solve_adaptive(double tint, double dt0, const std::string& dirout, int inter);
    void solve_adaptive(double tint, double dt0, std::size_t nsnap, const std::string& dirout);
    void solve_adaptive(double dt0, const std::vector<double>& tsnap, const std::string& dirout);

protected:
    /**
     * @brief Basic loop for adaptive stepping (no output, no counters).
     * @param tint   total integration time
     * @param dt0    initial time-step size
     * @param extra  whether to call after_step()
     */
    void solve_adaptive_impl(double tint, double dt0, bool extra = true);

    /**
     * @brief Single adaptive step that calls `Derived::adapt()`, `Derived::is_rejected()`,
     * and updates the solution/time accordingly.
     * @param dt    time-step
     * @param extra whether to call after_step() if accepted
     * @return true if step accepted, false if rejected
     */
    bool step_adaptive_impl(double dt, bool extra = true);

    /**
     * @brief Wrapper around dt_adapt() to do boundary checks (dt <= dtmax_ and no overshoot).
     * @param tend End of integration
     * @return New dt from derived's adapt function, clipped by dtmax_
     */
    double dt_adapt_clipped(double tend);

    /**
     * @brief Checks if we are done with the adaptive solve.
     * @param tend Integration end time
     */
    bool solve_done_adaptive(double tend);

protected:
    // Basic adaptive variables
    std::size_t n_reject_;  ///< count of rejected steps
    double      abstol_;    ///< absolute error tolerance
    double      reltol_;    ///< relative error tolerance
    double      dtmax_;     ///< maximum allowable time-step

    /**
     * @brief Store the solution in case a step is rejected and we need to revert.
     */
    std::vector<double> sol_prev_;
};

} // namespace DES

// Template definitions
#include "des_adaptive.ipp"

