#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../include/Methods/des_dopri54.hpp"

#ifndef DES_PROJECT_SOURCE_DIR
#define DES_PROJECT_SOURCE_DIR "."
#endif

// ---------------------------------------------------------------------------
// Lorenz system  dx/dt = σ(y−x),  dy/dt = x(ρ−z)−y,  dz/dt = xy − βz
// ---------------------------------------------------------------------------

struct LorenzSystem {
    double sigma = 10.0;
    double rho = 28.0;
    double beta = 8.0 / 3.0;

    void operator()(double /*t*/, const DES::Vec<3> &y, DES::Vec<3> &dydt) const
    {
        dydt[0] = sigma * (y[1] - y[0]);
        dydt[1] = y[0] * (rho - y[2]) - y[1];
        dydt[2] = y[0] * y[1] - beta * y[2];
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

    using Solver = DES::DoPri54<3>;
    using State = DES::Vec<3>;

    // Configure solver
    Solver solver;
    solver.options.rtol = 1.0e-9;
    solver.options.atol = 1.0e-12;
    solver.options.h_init = 1.0e-3;
    solver.options.h_max = 0.02;
    solver.options.save_history = true;
    solver.options.uniform_output = true;
    solver.options.output_points = 10000;

    // Initial conditions
    State y;
    y[0] = 1.0;
    y[1] = 1.0;
    y[2] = 1.0;

    LorenzSystem rhs;

    // Integrate
    constexpr double t0 = 0.0;
    constexpr double t1 = 100.0;

    const auto result = solver.solve(y, t0, t1, rhs);

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

    const fs::path csv_path = data_dir / "lorenz.csv";
    if (!write_csv(csv_path, solver.history()))
    {
        return EXIT_FAILURE;
    }

    // Summary
    const auto &st = solver.stats();
    std::cout << "wrote csv to:  " << fs::absolute(csv_path) << '\n'
              << "stored points: " << count << '\n'
              << "final state:   x=" << y[0] << "  y=" << y[1] << "  z=" << y[2] << '\n'
              << "accepted:      " << st.accepts << '\n'
              << "rejected:      " << st.rejects << '\n'
              << "rhs evals:     " << st.rhs_evals << '\n';

    return EXIT_SUCCESS;
}
