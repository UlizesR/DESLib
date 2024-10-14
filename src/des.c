#include "des.h"

// Initialize the DES structure
int DES_INIT(
    DES *des,
    void (*func)(const double, const double[], double[]),
    const size_t num_eqs,
    const double init_cond[],
    const DES_CONFIG config
) {
    if (des == NULL || func == NULL || init_cond == NULL) {
        fprintf(stderr, "DES_INIT: Invalid arguments.\n");
        return -1;
    }

    des->func = func;
    des->num_eqs = num_eqs;
    des->config = config;

    // Allocate and copy initial conditions
    des->init_cond = (double *)malloc(num_eqs * sizeof(double));
    if (des->init_cond == NULL) {
        fprintf(stderr, "DES_INIT: Memory allocation for initial conditions failed.\n");
        return -1;
    }
    for (size_t i = 0; i < num_eqs; i++) {
        des->init_cond[i] = init_cond[i];
    }

    des->solutions = NULL;
    des->errors = NULL;
    des->times = NULL;
    des->samples = 0;

    return 0;
}

// Simple Euler method solver
int DES_SOLVE(DES *des, const double t0, const double tf, const int samples, DES_METHOD method) 
{
    if (des == NULL || des->func == NULL || des->init_cond == NULL) {
        fprintf(stderr, "DES_SOLVE: DES not properly initialized.\n");
        return -1;
    }

    if (samples <= 0) {
        fprintf(stderr, "DES_SOLVE: Number of samples must be positive.\n");
        return -1;
    }

    des->samples = samples;
    double dt = (tf - t0) / samples;

    // Allocate memory for time points
    des->times = (double *)malloc(samples * sizeof(double));
    if (des->times == NULL) {
        fprintf(stderr, "DES_SOLVE: Memory allocation for times failed.\n");
        return -1;
    }

    // Allocate contiguous memory for solutions and errors
    des->solutions = (double *)malloc(samples * des->num_eqs * sizeof(double));
    if (des->solutions == NULL) {
        fprintf(stderr, "DES_SOLVE: Memory allocation for solutions failed.\n");
        free(des->times);
        return -1;
    }

    des->errors = (double *)malloc(samples * des->num_eqs * sizeof(double));
    if (des->errors == NULL) {
        fprintf(stderr, "DES_SOLVE: Memory allocation for errors failed.\n");
        free(des->times);
        free(des->solutions);
        return -1;
    }

    // Initialize the first time point and solution
    des->times[0] = t0;
    for (size_t i = 0; i < des->num_eqs; i++) {
        des->solutions[i] = des->init_cond[i];
        des->errors[i] = 0.0; // Initialize errors to zero
    }

    // Temporary arrays for computation
    double *current_y = (double *)malloc(des->num_eqs * sizeof(double));
    double *dydt = (double *)malloc(des->num_eqs * sizeof(double));
    if (current_y == NULL || dydt == NULL) {
        fprintf(stderr, "DES_SOLVE: Memory allocation for temporary arrays failed.\n");
        free(des->times);
        free(des->solutions);
        free(des->errors);
        free(current_y);
        free(dydt);
        return -1;
    }

    // Copy initial conditions to current_y
    for (size_t i = 0; i < des->num_eqs; i++) {
        current_y[i] = des->init_cond[i];
    }
 


    free(current_y);
    free(dydt);

    return 0;
}

// Free allocated memory in DES structure
void DES_FREE(DES *des) {
    if (des == NULL) {
        return;
    }

    if (des->init_cond != NULL) {
        free(des->init_cond);
        des->init_cond = NULL;
    }

    if (des->times != NULL) {
        free(des->times);
        des->times = NULL;
    }

    if (des->solutions != NULL) {
        free(des->solutions);
        des->solutions = NULL;
    }

    if (des->errors != NULL) {
        free(des->errors);
        des->errors = NULL;
    }

    des->samples = 0;
}


