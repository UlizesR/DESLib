#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>  
#include <cstdlib>   

namespace DES {

/*!
 * @brief Check if a file can be opened for writing. 
 * Throws an exception if it cannot be opened.
 */
inline void des_check_write(const std::string &filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
}

/*!
 * @brief Write a vector of values to a binary file.
 *
 * @tparam T Type of elements in the vector
 * @param filename Name of the file to write to
 * @param data Vector of values to write
 */
template<typename T>
inline void des_write(const std::string &filename, const std::vector<T> &data) {
    // Check that we can open the file
    des_check_write(filename);

    // Write the data in binary mdes
    std::ofstream ofs(filename, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(T));

    // Check for write errors
    if (!ofs) {
        throw std::runtime_error("Error writing to file: " + filename);
    }
}

/*!
 * @brief Print an error message and exit with failure.
 *
 * @param msg The message to print
 */
inline void des_print_exit(const std::string &msg) {
    std::cerr << "\ndes FAILURE: " << msg << "\n\n";
    std::exit(EXIT_FAILURE);
}

/*!
 * @brief Convert a long integer to string (C++17's std::to_string).
 */
inline std::string des_int_to_string(long i) {
    return std::to_string(i);
}

} // namespace DES
