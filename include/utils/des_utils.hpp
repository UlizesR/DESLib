#pragma once

#include <cmath>
#include <algorithm>

namespace DES {

/*!
 * @brief Returns the maximum of two double values.
 */
inline double des_max2(double a, double b) {
    return std::max(a, b);
}

/*!
 * @brief Returns the minimum of two double values.
 */
inline double des_min2(double a, double b) {
    return std::min(a, b);
}

/*!
 * @brief Checks if two numbers are very close to each other within a threshold.
 */
inline bool des_is_close(double a, double b, double thresh) {
    double absd = std::fabs(a - b);
    double absa = std::fabs(a);
    double absb = std::fabs(b);

    // Check relative difference against a threshold
    return ((absd / absa < thresh) && (absd / absb < thresh));
}

} // namespace DES
