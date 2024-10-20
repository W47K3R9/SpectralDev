/**
 * Author: Lucas Scheidt
 * Date: 13.10.24
 *
 * Description: Testing of the domain specific values.
 *
 */
#pragma once
#include "SpctDomainSpecific.h"
#include <iostream>

namespace LBTS::Spectral
{
inline void test_domain_specific_functions_and_values()
{
    std::cout << "Testing domain specific functions and values..." << std::endl;
    // Won't compile if max_num_of_samples was invalid...
    constexpr auto assert_min_is_valid = BoundedPowTwo_v<uint16_t, min_num_of_samples>;
    std::cout << "Minimum number of samples is: " << assert_min_is_valid << std::endl;
    constexpr auto assert_max_is_valid = BoundedPowTwo_v<uint16_t, max_num_of_samples>;
    std::cout << "Maximum number of samples is: " << assert_max_is_valid << std::endl;

    // See if numbers align
    static_assert(BoundedDegTwo_v<min_pow_two_degree> == assert_min_is_valid);
    static_assert(BoundedDegTwo_v<max_pow_two_degree> == assert_max_is_valid);
    static_assert(BoundedDegTwo_v<8> == 1u << 8);
    static_assert(is_bounded_degree(1u) == false);
    static_assert(is_bounded_degree(4u) == true);
    static_assert(is_bounded_degree(17u) == false);

    // basic checks
    static_assert(pow_two_value_of_degree(0u) == 1);
    static_assert(pow_two_value_of_degree(4u) == 16u);
    static_assert(pow_two_value_of_degree(5u) == 32u);

    // type checks the names may be inapropriate from system to system...
    auto check_unsigned_char = pow_two_value_of_degree<uint8_t>(5);
    static_assert(std::is_same_v<decltype(check_unsigned_char), uint8_t>);
    auto check_unsigned_short = pow_two_value_of_degree<uint16_t>(5);
    static_assert(std::is_same_v<decltype(check_unsigned_short), uint16_t>);
    auto check_unsigned = pow_two_value_of_degree<uint32_t>(5);
    static_assert(std::is_same_v<decltype(check_unsigned), uint32_t>);
    auto check_unsigned_long = pow_two_value_of_degree<uint64_t>(5);
    static_assert(std::is_same_v<decltype(check_unsigned_long), uint64_t>);

    // check that exaggerated values stick to the max of the appropriate type
    auto check_type_0 = pow_two_value_of_degree<uint64_t>(65);
    assert(check_type_0 == 1ull << 63);
    static_assert(std::is_same_v<decltype(check_type_0), uint64_t>);
    auto check_type_1 = pow_two_value_of_degree<uint32_t>(65);
    assert(check_type_1 == 1ull << 31);
    static_assert(std::is_same_v<decltype(check_type_1), uint32_t>);
    auto check_type_2 = pow_two_value_of_degree<uint16_t>(65);
    assert(check_type_2 == 1ull << 15);
    static_assert(std::is_same_v<decltype(check_type_2), uint16_t>);
    auto check_type_3 = pow_two_value_of_degree<uint8_t>(65);
    assert(check_type_3 == 1ull << 7);
    static_assert(std::is_same_v<decltype(check_type_3), uint8_t>);

    // ceck clipping function
    auto actual1 = clip_to_lower_pow_two<uint8_t>(0xFF);
    static_assert(std::is_same_v<decltype(actual1), uint8_t>, "Return type mismatch for uint8_t");
    assert(actual1 == 1ull << 7);
    auto actual2 = clip_to_lower_pow_two<uint16_t>(0xFFFF);
    static_assert(std::is_same_v<decltype(actual2), uint16_t>, "Return type mismatch for uint16_t");
    assert(actual2 == 1ull << 15);
    auto actual3 = clip_to_lower_pow_two<uint32_t>(0xFFFFFFFF);
    static_assert(std::is_same_v<decltype(actual3), uint32_t>, "Return type mismatch for uint32_t");
    assert(actual3 == 1ull << 31);
    auto actual4 = clip_to_lower_pow_two<uint64_t>(0xFFFFFFFFFFFFFFFF);
    static_assert(std::is_same_v<decltype(actual4), uint64_t>, "Return type mismatch for uint64_t");
    assert(actual4 == 1ull << 63);

    static_assert(pow_two_value_of_degree(32u) == 1u << 31);
    static_assert(pow_two_value_of_degree(63ull) == 1ull << 63);
    static_assert(pow_two_value_of_degree(64ull) == 1ull << 63);
    static_assert(pow_two_value_of_degree<size_t>(64) == 1ull << 63);
    static_assert(pow_two_value_of_degree<uint16_t>(64) == 1ull << 15);
    static_assert(pow_two_value_of_degree<uint16_t>(14) == 1ull << 14);
    static_assert(pow_two_value_of_degree<uint32_t>(32) == 1u << 31);
    static_assert(pow_two_value_of_degree<uint8_t>(7) == 128u);
    static_assert(pow_two_value_of_degree<uint8_t>(8) == 128u);
    static_assert(pow_two_value_of_degree<uint64_t>(63) == 1ul << 63);
    static_assert(pow_two_value_of_degree<uint64_t>(64) == 1ul << 63);

    static_assert(clip_to_lower_pow_two<uint32_t>(24000) == 16384);
    static_assert(clip_to_lower_pow_two<uint32_t>(24000) == 16384);
    static_assert(clip_to_lower_pow_two<uint8_t>(167) == 128);
    static_assert(pow_two_value_of_degree<uint64_t>(34) == 17179869184);
    static_assert(clip_to_lower_pow_two<uint64_t>(17179869190) == 17179869184);
    static_assert(clip_to_lower_pow_two<size_t>(17179869190) == 17179869184);
    static_assert(clip_to_lower_bounded_pow_two<size_t>(17179869190) == 16384);
    static_assert(clip_to_lower_bounded_pow_two<size_t>(0) == 16);

    // 2565 = 0b1010'0000'0101
    // clip without bunds: 4
    // clip with bounds: 16
    static_assert(clip_to_lower_pow_two<uint8_t>(2565) == static_cast<uint8_t>(4));
    static_assert(clip_to_lower_bounded_pow_two<uint16_t>(2565) == 2048);

    static_assert(BoundedDegTwo_v<14, uint16_t> == 16384);
    static_assert(BoundedDegTwo_v<7, uint16_t> == 128);
    static_assert(BoundedDegTwo_v<6, uint16_t> == 64);
    // constexpr auto invalid_type = BoundedDegTwo<14, uint8_t>::degree;
    // constexpr auto invalid_degree = BoundedDegTwo<15, uint16_t>::value;
    // static_assert(BoundedPowTwo_v<uint8_t, 256> == 256);
};
} // namespace LBTS::Spectral