/**
 * Author: Lucas Scheidt
 * Date: 28.10.2024
 *
 * Purpose: This file contains domain specific declarations of concepts and types needed for the audio plugin to
 * implement.
 */

#pragma once
#include <algorithm>
#include <span>

namespace LBTS::Spectral
{
/**
 * @section this header contains:
 * 1. Definition of constants regarding the maximum processing capability of the plugins in terms of samples.
 * These are not explicitely checked, but have been tested in the SpctDomainSpecificTest.h file, just to be sure to avoid
 * misbehaviours.
 *
 * 2. Implementation of several compile- or runtime checks if the specified limits are respected and if a number is
 * actually a power of two.
 *
 * 3. Implementation of structs to provide a conveniant domain specific way to declare variables that are guaranteed
 * to be in the valid constraints. The format is loosely related to the way the STL defines it's type infos.
 * e.g. the value of a bounded power of two (BoundedPowTwo) is accessed either directly with BoundedPowTwo::value
 * or with the "_v" suffix BoundedPowTwo_v
 *
 * @note
 * The sampling rate of a project can change at runtime. It's not recommended though to use an arbitrary large amount of
 * samples (processing power is not infinite!). For now, the maximum is set to about 341 ms at 48 kHz. That could change
 * in the future but for now it's a good starting point.
 * Furthermore, a sufficient amount of samples is needed for an accurate transformation.
 * Frequency resolution = sampling freq (fs) / number of samples (n)
 * 16 samples leads to a miserable 3 kHz resolution with 48 kHz fs but let's see how it sounds, maybe this leeds to
 * interesting experimental results.
 *
 * Function summary:
 * - is_bounded_degree
 * - is_bounded_no_of_samples
 * - is_bounded_pow_two
 * - pow_two_value_of_degree
 * - clip_to_lower_pow_two
 * - clip_to_lower_bounded_pow_two
 *
 * Struct summary:
 * - BoundedPowTwo (value access with BoundedPowTwo_v)
 * - BoundedDegTwo (value access with BoundedDegTwo_v)
 */

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief plugin specific constants
constexpr uint32_t min_pow_two_degree = 4;
constexpr uint32_t max_pow_two_degree = 14;
constexpr uint32_t min_num_of_samples = 16;
constexpr uint32_t max_num_of_samples = 16384;

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief this concept enforces the use of unsigned integer values with a maximum size of 8 Byte (64 Bit).
template <typename T>
concept Spct_Unsigned = std::is_unsigned_v<T> && sizeof(T) <= 8;

/// @brief further constraint for the BoundedDegTwo and BoundedPowTwo types, so that the correct maximum can always
/// be assigned.
/// Spct_U_GE_16 = Spectral Unsigned Greater or Equal to 16 Byte.
template <typename T>
concept Spct_U_GE_16 = std::is_unsigned_v<T> && sizeof(T) <= 8 && sizeof(T) > 1;

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief check if the degree to calculate is in an appropriate range (2^14 = 16384 -> actual max)
template <Spct_Unsigned T>
constexpr bool is_bounded_degree(const T i_degree) noexcept
{
    return i_degree <= max_pow_two_degree && i_degree >= min_pow_two_degree;
}

/// @brief compile- or runtime check if a given number of samples is in the valid range of this plugin.
template <Spct_Unsigned T>
constexpr bool is_bounded_no_of_samples(T i_samples) noexcept
{
    return i_samples >= min_num_of_samples && i_samples <= max_num_of_samples;
}

/// @brief checks the range, as well as that the number is actually a power of two.
template <Spct_Unsigned T>
constexpr bool is_bounded_pow_two(const T i_number) noexcept
{
    return is_bounded_no_of_samples(i_number) && (i_number & (i_number - 1)) == 0;
}

/// @brief  funcution to determine the value of a degree of a power of two can be used at compile- or runtime.
/// Note that the maximum value can't be greater than the maximum representable by the given type and at max 2^63 so
/// no overflow is possible (as opposed to the regular << operator).
template <Spct_Unsigned T>
constexpr T pow_two_value_of_degree(T i_degree) noexcept
{
    // make sure the one to shift left has the same type as i_degree.
    constexpr T correct_one = 1;
    // safety check to prevent overflows.
    if (i_degree >= sizeof(T) * 8)
    {
        return (correct_one << sizeof(T) * 8 - 1);
    }
    return correct_one << i_degree;
}

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief This struct is mainly used as a security check to be sure to initialize values according to a real
/// power of two that is in the valid range of the samples used by this plugin. To assure that the maximum is always
/// a valid value, the type given has to have at least 16 Bit.
/// This is a compile-time check and NOT intended to be an object.
template <Spct_U_GE_16 T, T pot>
    requires(is_bounded_pow_two(pot))
struct BoundedPowTwo
{
    BoundedPowTwo() = delete;
    static constexpr T value = pot;
};

/// @brief and for conveniance you can access the value directly.
template <Spct_U_GE_16 T, T pot>
constexpr T BoundedPowTwo_v = BoundedPowTwo<T, pot>::value;

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief like BoundedPowTwo but you specify a degree instead of a direct value. The rule with at least 16 Bit also
/// applies to this struct!
template <uint8_t deg, Spct_U_GE_16 T = uint32_t>
    requires(is_bounded_degree(deg))
struct BoundedDegTwo
{
    BoundedDegTwo() = delete;
    static constexpr uint8_t degree = deg;
    static constexpr T value = pow_two_value_of_degree<T>(deg);
};

/// @brief access the actual number of samples that resulted from the given degree.
template <uint8_t deg, Spct_U_GE_16 T = uint32_t>
constexpr T BoundedDegTwo_v = BoundedDegTwo<deg, T>::value;

/* -------------------------------------------------------------------------------------------------------------------*/
/// @brief clamp the given number to the closest lower power of two.
/// Example:
/// input:  19 (0b0000'1011)
/// output: 16 (0b0000'1000)
/// If the number given is greater than the range of the desired type, only the valid bits are taken into account.
/// The number get's implicitely casted by the templatization.
/// Example:
/// input: clip_to_lower_pow_two<uint8_t>(2565) --> 2565 = (0b1010'0000'0101)
/// cast: 5 (0b0000'0101)
/// output: 4 (0b0000'0100)
template <Spct_Unsigned T>
constexpr T clip_to_lower_pow_two(T i_value) noexcept
{
    if (i_value == 0)
    {
        return 1;
    }
    constexpr T correct_sized_one = 1;
    constexpr T size_of_value = correct_sized_one << sizeof(T) * 8 - 1;
    // won't be greater than 64 (ull) due to Spct_Unsigned constraint and even 128 fits in uint8_t.
    uint8_t first_occurance = 0;
    while (!((i_value << first_occurance) & size_of_value))
    {
        ++first_occurance;
    }
    return correct_sized_one << sizeof(T) * 8 - 1 - first_occurance;
}

/// @brief like above but only for valid plugin values. To guarantee that the max value can be assigned, a passed type
/// has to be at least 16 bytes large!
template <Spct_U_GE_16 T>
constexpr T clip_to_lower_bounded_pow_two(T i_value) noexcept
{
    if (i_value <= min_num_of_samples)
    {
        return BoundedPowTwo_v<T, min_num_of_samples>;
    }
    if (i_value >= max_num_of_samples)
    {
        return BoundedPowTwo_v<T, max_num_of_samples>;
    }
    return clip_to_lower_pow_two(i_value);
}

} // namespace LBTS::Spectral
