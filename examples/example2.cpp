#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/Methods/des_dopri54.hpp"

#ifndef DES_PROJECT_SOURCE_DIR
#define DES_PROJECT_SOURCE_DIR "."
#endif

// ---------------------------------------------------------------------------
// Two-variable circadian clock DDE (Scheper-style minimal model)
//
//   dm/dt = r_m / (1 + (p / k)^n) - q_m m
//   dp/dt = r_p m(t - tau)^m_hill - q_p p
//
// m: per mRNA concentration
// p: effective PER protein concentration
//
// This example is intentionally more complex than the scalar Mackey-Glass demo:
// it uses a vector-valued state, delayed coupling across components, and a delay
// long enough to produce self-sustained oscillations with a circadian-scale
// period.
// ---------------------------------------------------------------------------

struct CircadianClockDDE {
    double r_m = 1.0;
    double q_m = 0.21;
    double r_p = 1.0;
    double q_p = 0.21;
    double k = 1.0;
    double n = 2.0;
    double m_hill = 3.0;
    double tau = 4.0;

    void operator()(double t, const DES::Vec<2> &y, const DES::DelayHistoryView<2> &hist, DES::Vec<2> &dydt) const
    {
        const double m = y[0];
        const double p = y[1];
        const double m_tau = hist(0, t - tau);
        const double p_scaled = p / k;

        dydt[0] = r_m / (1.0 + p_scaled * p_scaled) - q_m * m;
        dydt[1] = r_p * m_tau * m_tau * m_tau - q_p * p;
    }
};

// ---------------------------------------------------------------------------
// Write the solver history to a CSV file.
// Returns false and prints a message on I/O failure.
// ---------------------------------------------------------------------------

template <typename History>
[[nodiscard]] bool write_csv(const std::filesystem::path &path, const History &history)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        std::cerr << "error: could not open output file: " << path << '\n';
        return false;
    }

    file << std::fixed << std::setprecision(10);
    file << "t,m,p,error_norm,h\n";

    const auto count = history.t.size();
    for (std::size_t i = 0; i < count; ++i)
    {
        file << history.t[i] << ',' << history.y[i][0] << ',' << history.y[i][1] << ',' << history.error[i] << ',' << history.h[i] << '\n';
    }

    return file.good();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    namespace fs = std::filesystem;

    using Solver = DES::DoPri54<2>;
    using State = DES::Vec<2>;

    CircadianClockDDE rhs;

    // Configure solver
    Solver solver;
    solver.options.rtol = 1.0e-8;
    solver.options.atol = 1.0e-10;
    solver.options.h_init = 1.0e-3;
    solver.options.h_max = 0.25;
    solver.options.min_delay = rhs.tau;
    solver.options.save_history = true;
    solver.options.uniform_output = true;
    solver.options.output_points = 4000;

    // Initial condition at t0
    State y;
    y[0] = 0.67;  // mRNA
    y[1] = 14.8;  // protein

    // Prehistory: constant state on t <= 0.
    // This keeps the example self-contained while still exercising the delay
    // machinery through the delayed mRNA -> protein coupling.
    const std::vector<double> max_delays = {rhs.tau, rhs.tau};
    const std::vector<double> init_conds = {y[0], y[1]};
    const std::vector<std::function<double(double)>> prehistory = {
        [m0 = y[0]](double /*t*/) { return m0; },
        [p0 = y[1]](double /*t*/) { return p0; },
    };

    DES::History<double, double> dde_hist(2, 0.0, 0.0, max_delays, init_conds, prehistory);

    // Integrate long enough to capture several circadian-scale oscillations.
    constexpr double t0 = 0.0;
    constexpr double t1 = 240.0;

    const auto result = solver.solve(y, t0, t1, rhs, dde_hist);

    if (!result.ok())
    {
        std::cerr << "solve failed with status " << static_cast<int>(result.status) << '\n';
        return EXIT_FAILURE;
    }

    const auto count = solver.history_size();
    if (count != solver.options.output_points)
    {
        std::cerr << "expected " << solver.options.output_points << " output points, got " << count << '\n';
        return EXIT_FAILURE;
    }

    // Write output
    const fs::path data_dir = fs::path(DES_PROJECT_SOURCE_DIR) / "examples" / "data";
    fs::create_directories(data_dir);

    const fs::path csv_path = data_dir / "circadian_clock_dde.csv";
    if (!write_csv(csv_path, solver.history()))
    {
        return EXIT_FAILURE;
    }

    // Summary
    const auto &st = solver.stats();
    std::cout << "wrote csv to:  " << fs::absolute(csv_path) << '\n' << "stored points: " << count << '\n' << "final state:   m=" << y[0] << "  p=" << y[1] << '\n' << "accepted:      " << st.accepts << '\n' << "rejected:      " << st.rejects << '\n' << "rhs evals:     " << st.rhs_evals << '\n';

    return EXIT_SUCCESS;
}
