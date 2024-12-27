#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <optional>   

#include "../utils/des_io.hpp"    
#include "../utils/des_util.hpp"

namespace DES
{

/**
 * @brief CRTP base class for ODE (DES) solvers.
 *
 * @tparam Derived The derived solver class (the method).
 * @tparam Func    The function/functor describing the ODE system f(x).
 *
 * The derived class should implement:
 * - `void step_impl(double dt)` – the actual integrator step
 * - `void des_fun(const std::vector<double>& solin, std::vector<double>& fout)` – ODE system
 * - optionally, `void des_jac(const std::vector<double>& solin,
 *                             std::vector<std::vector<double>>& Jout)` for an analytical Jacobian
 */
template <typename Derived, typename Func>
class DesBase {
public:
    /**
     * @brief Constructor
     * @param neq       Number of equations
     * @param need_jac  Whether a Jacobian is needed
     * @param func      The ODE system functor/object
     */
    DesBase(std::size_t neq, bool need_jac, const Func& func);

    /**
     * @brief Destructor
     */
    ~DesBase() = default;

    // -------------------------
    // Getters
    // -------------------------
    inline const std::string& get_name()        const { return name_; }
    inline const std::string& get_method()      const { return method_; }
    inline std::optional<std::string> get_dirout() const { return dirout_; }
    inline bool               get_verbose()     const { return verbose_; }
    inline bool               get_silent_snap() const { return silent_snap_; }
    inline std::size_t        get_neq()         const { return neq_; }
    inline double             get_t()           const { return t_; }
    inline double             get_dt()          const { return dt_; }
    inline const std::vector<double>& get_sol() const { return sol_; }
    inline double             get_sol(std::size_t i) const { return sol_[i]; }
    inline std::size_t        get_nstep()       const { return nstep_; }
    inline std::size_t        get_neval()       const { return neval_; }
    inline std::size_t        get_icheck()      const { return icheck_; }
    inline std::size_t        get_nJac()        const { return nJac_; }

    // -------------------------
    // Setters
    // -------------------------
    inline void set_t(double t)                        { t_ = t; }
    inline void set_sol(std::size_t i, double x)       { sol_[i] = x; }
    void set_sol(std::vector<double> s);
    inline void set_name(const std::string& name)      { name_ = name; }
    inline void set_method(const std::string& m)       { method_ = m; }
    inline void set_verbose(bool verbose)              { verbose_ = verbose; }
    inline void set_silent_snap(bool silent_snap)      { silent_snap_ = silent_snap; }
    inline void set_icheck(std::size_t icheck)         { icheck_ = icheck; }

    /**
     * @brief Optional: set output directory (std::optional).
     */
    inline void set_dirout(const std::string& d)       { dirout_ = d; }
    inline void clear_dirout()                         { dirout_.reset(); }

    // ----------------------------------------------------------------
    // Public interface: step() calls Derived::step_impl(dt) under CRTP
    // ----------------------------------------------------------------
    void step(double dt, bool extra = true);

    // ----------------------------------------------------------------
    // Solve routines
    // ----------------------------------------------------------------
    void solve_fixed(double tint, double dt, bool extras = true);
    void solve_fixed(double tint, double dt, const std::string& dirout, int inter = 1);
    void solve_fixed(double tint, double dt, std::size_t nsnap, const std::string& dirout);
    void solve_fixed(double dt, const std::vector<double>& tsnap, const std::string& dirout);

    /**
     * @brief Reset the solver's time and solution
     * @param t   New time
     * @param sol New solution vector
     */
    void reset(double t, const std::vector<double>& sol);

protected:
    // ----------------------------------------------------------------
    // Protected "helpers" (CRTP calls)
    // ----------------------------------------------------------------
    void des_fun_(const std::vector<double>& solin, std::vector<double>& fout);
    void des_jac_(const std::vector<double>& solin, std::vector<std::vector<double>>& Jout);
    void numerical_jac(const std::vector<double>& solin,
                       std::vector<std::vector<double>>& Jout);

    // ----------------------------------------------------------------
    // "Extra" user-overridable hooks
    // ----------------------------------------------------------------
    inline void before_solve()                { /* Derived may override */ }
    inline void after_step(double)            { /* Derived may override */ }
    inline void after_capture(double)         { /* Derived may override */ }
    inline void after_snap(const std::string&, std::size_t, double) { /* Derived may override */ }
    inline void after_solve()                 { /* Derived may override */ }

    // ----------------------------------------------------------------
    // Internal support
    // ----------------------------------------------------------------
    void snap(const std::string& dirout, std::size_t isnap, double tsnap);
    void solve_fixed_(double tint, double dt, bool extra = true);

    bool solve_done(double dt, double tend);
    void check_sol_integrity();
    void check_pre_solve(double tint, double dt);
    void check_pre_snaps(double dt, const std::vector<double>& tsnap);

private:
    // CRTP: get the derived class
    Derived& derived() {
        return *static_cast<Derived*>(this);
    }
    const Derived& derived() const {
        return *static_cast<const Derived*>(this);
    }

protected:
    // Basic solver variables
    std::string name_;           
    std::string method_;         

    // (#13) Store the output directory as std::optional
    std::optional<std::string> dirout_;         

    bool        verbose_;        
    bool        silent_snap_;    
    std::size_t neq_;           
    double      t_;             
    double      dt_;            
    std::size_t nstep_;         
    std::size_t neval_;         
    std::size_t icheck_;        

    std::vector<double> sol_;   

    // Jacobian support
    bool need_jac_;
    std::size_t nJac_;
    double absjacdel_;
    double reljacdel_;
    std::vector<std::vector<double>> Jac_; 
    std::vector<double> f_; ///< Temporary for numeric Jacobian
    std::vector<double> g_; ///< Temporary for numeric Jacobian

    // The ODE system function or functor
    Func func_;
};

} // namespace DES

// Template definitions in separate file
#include "des_base.ipp"
