#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../include/Methods/des_rossenbrock.hpp"

#ifndef DES_PROJECT_SOURCE_DIR
#define DES_PROJECT_SOURCE_DIR "."
#endif

// ---------------------------------------------------------------------------
// Robertson stiff kinetics problem
//
//   y1' = -0.04 y1 + 1e4 y2 y3
//   y2' =  0.04 y1 - 1e4 y2 y3 - 3e7 y2^2
//   y3' =  3e7 y2^2
//
// This is a classic stiff ODE system, which makes it a good fit for the
// Rosenbrock solver. The example also provides an analytical Jacobian so the
// solver does not need to finite-difference one internally.
// ---------------------------------------------------------------------------

struct RobertsonSystem {
    void operator()(double /*t*/, const DES::Vec<3> &y, DES::Vec<3> &dydt) const
    {
        const double y1 = y[0];
        const double y2 = y[1];
        const double y3 = y[2];

        dydt[0] = -0.04 * y1 + 1.0e4 * y2 * y3;
        dydt[1] = 0.04 * y1 - 1.0e4 * y2 * y3 - 3.0e7 * y2 * y2;
        dydt[2] = 3.0e7 * y2 * y2;
    }

    void jacobian(double /*t*/, const DES::Vec<3> &y, Eigen::Matrix<double, 3, 3> &J) const
    {
        const double y2 = y[1];
        const double y3 = y[2];

        J(0, 0) = -0.04;
        J(0, 1) = 1.0e4 * y3;
        J(0, 2) = 1.0e4 * y2;

        J(1, 0) = 0.04;
        J(1, 1) = -1.0e4 * y3 - 6.0e7 * y2;
        J(1, 2) = -1.0e4 * y2;

        J(2, 0) = 0.0;
        J(2, 1) = 6.0e7 * y2;
        J(2, 2) = 0.0;
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
    file << "t,y0,y1,y2,error_norm,h\n";

    const auto count = history.t.size();
    for (std::size_t i = 0; i < count; ++i)
    {
        file << history.t[i] << ',' << history.y[i][0] << ',' << history.y[i][1] << ',' << history.y[i][2] << ',' << history.error[i] << ',' << history.h[i] << '\n';
    }

    return file.good();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    namespace fs = std::filesystem;

    using Solver = DES::Rosenbrock4<3>;
    using State = DES::Vec<3>;

    RobertsonSystem rhs;

    Solver solver;
    solver.options.rtol = 1.0e-8;
    solver.options.atol = 1.0e-10;
    solver.options.h_init = 2.0e-4;
    solver.options.h_max = 10.0;
    solver.options.save_history = true;
    solver.options.uniform_output = true;
    solver.options.output_points = 1000;

    State y;
    y[0] = 1.0;
    y[1] = 0.0;
    y[2] = 0.0;

    constexpr double t0 = 0.0;
    constexpr double t1 = 100.0;

    const auto result = solver.solve(y, t0, t1, rhs);

    if (!result.ok())
    {
        const auto &st = solver.stats();
        std::cerr << "solve failed with status " << static_cast<int>(result.status) << " at t=" << result.t_final << " after " << st.steps << " steps"
                  << " (accepted=" << st.accepts << ", rejected=" << st.rejects << ", rhs evals=" << st.rhs_evals << ")\n";
        return EXIT_FAILURE;
    }

    const auto count = solver.history_size();
    if (count != solver.options.output_points)
    {
        std::cerr << "expected " << solver.options.output_points << " output points, got " << count << '\n';
        return EXIT_FAILURE;
    }

    const fs::path data_dir = fs::path(DES_PROJECT_SOURCE_DIR) / "examples" / "data";
    fs::create_directories(data_dir);

    const fs::path csv_path = data_dir / "robertson_rossenbrock.csv";
    if (!write_csv(csv_path, solver.history()))
    {
        return EXIT_FAILURE;
    }

    const auto &st = solver.stats();
    std::cout << "wrote csv to:  " << fs::absolute(csv_path) << '\n'
              << "stored points: " << count << '\n'
              << "final state:   y1=" << y[0] << "  y2=" << y[1] << "  y3=" << y[2] << '\n'
              << "mass sum:      " << (y[0] + y[1] + y[2]) << '\n'
              << "accepted:      " << st.accepts << '\n'
              << "rejected:      " << st.rejects << '\n'
              << "rhs evals:     " << st.rhs_evals << '\n';

    return EXIT_SUCCESS;
}
