/**
 * Author: Lucas Scheidt
 * Date: 13.10.24
 *
 * Description: Testing of the domain specific values.
 *
 */

#pragma once
#include "SpctDomainSpecific.h"
#include "SpctPowTwoTest.h"
#include <iostream>

namespace LBTS::Spectral
{
inline void test_domain_specific_functions_and_values()
{
    std::cout << "Testing domain specific functions and values..." << std::endl;
    // Won't compile if max_num_of_samples was invalid...
    constexpr auto assert_min_is_valid = BoundedPowTwo_v<unsigned, min_num_of_samples>;
    std::cout << "Minimum number of samples is: " << assert_min_is_valid << std::endl;
    constexpr auto assert_max_is_valid = BoundedPowTwo_v<unsigned, max_num_of_samples>;
    std::cout << "Maximum number of samples is: " << assert_max_is_valid << std::endl;

    // See if numbers align
    static_assert(BoundedDegTwo_v<min_pow_two_degree> == assert_min_is_valid);
    static_assert(BoundedDegTwo_v<max_pow_two_degree> == assert_max_is_valid);
    static_assert(BoundedDegTwo_v<8> == 1u << 8);
    static_assert()


};
} // namespace LBTS::Spectral