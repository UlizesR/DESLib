#ifndef DES_STRUCTS_H
#define DES_STRUCTS_H

// Configuration structure for the solver
typedef struct {
    double abs_tol;   // Absolute tolerance
    double rel_tol;   // Relative tolerance
    int max_steps;    // Maximum number of steps
    double h0;        // Initial step size
    double hmin;      // Minimum step size
    double hmax;      // Maximum step size
} DES_CONFIG;

// Main DES structure
typedef struct {
    void (*func)(const double, const double[], double[]); // Function to compute derivatives
    size_t num_eqs;                                        // Number of equations
    int samples;                                           // Number of sample points
    doume t;                                               // Current time
    double *solutions;                                     // Solutions array (contiguous memory)
    double *errors;                                        // Errors array (contiguous memory)
    double *times;                                         // Time points array
    double *init_cond;                                     // Initial conditions
    DES_CONFIG config;                                     // Solver configuration
} DES;

typedef enum {
    DES_METHOD_EULER,
    DES_METHOD_RK4,
    DES_METHOD_RK45,
    DES_METHOD_RK78
} DES_METHOD;

#endif // DES_STRUCTS_H
