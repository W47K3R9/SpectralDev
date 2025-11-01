/**
 * Author: Lucas Scheidt
 * Date: 28.10.2024
 *
 * Purpose: This file contains domain specific declarations of concepts and types needed for the audio plugin to
 * implement.
 */

#pragma once
#include <bit>
#include <complex>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <numbers>

namespace LBTS::Spectral
{
/**
 * @section this header contains:
 * 1. Definition of constants regarding the maximum processing capability of the plugins in terms of samples.
 * These are not explicitely checked, but have been tested in the SpctDomainSpecificTest.h file, just to be sure to
 * avoid misbehaviours.
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
 * Concept summary:
 * - FloatingPt
 * - UnsignedL64
 * - UnsignedGE16
 *
 * Function summary:
 * - is_bounded_degree
 * - is_bounded_no_of_samples
 * - is_bounded_pow_two
 * - pow_two_value_of_degree
 * - degree_of_pow_two_value
 * - clip_to_lower_bounded_pow_two
 *
 * Struct summary:
 * - BoundedPowTwo (value access with BoundedPowTwo_v)
 * - BoundedDegTwo (value access with BoundedDegTwo_v)
 *
 * Type aliases:
 * - IndexValueArr: array of index value pairs
 * - ComplexArr: array containing complex numbers
 */

/// @brief A class that manages synchronous operation between threads. It is intended to be used to manage the proper
/// calculation of the fourier map while guaranteeing that the buffer used to create the fft remains static during
/// the calculation. Further it is used for the tuning of the oscillators.
struct SyncPrimitives
{
    /// @brief Used for signalling between threads.
    std::condition_variable signalling_cv;
    /// @brief Gets locked to wait on the condition variable.
    std::mutex signalling_mtx;
    /// @brief For producer consumer patterns this can be used to avoid mangling with shared objects.
    std::atomic_bool action_done;
    /// @brief Can be used for components that need to behave differently on a common condition. Like a shared switch,
    /// this could be extended to a list of operations if the need arises.
    std::atomic_bool common_ondition;
};

/// @brief This concept enforces the use of floating point types (float, double, long double).
/// @tparam T: Type to constrain.
template <typename T>
concept FloatingPt = std::is_floating_point_v<T>;

/// @brief Used to select the waveform for resynthesizing.
enum class OscWaveform : uint8_t
{
    SINE,
    TRIANGLE,
    SAW,
    SQUARE
};

/// @brief Plugin specific constants
constexpr uint32_t min_pow_two_degree = 0;
constexpr uint32_t max_pow_two_degree = 11;
constexpr uint32_t min_num_of_samples = 1;
constexpr uint32_t max_num_of_samples = 2048;
constexpr uint32_t max_oscillators = 46;

template <FloatingPt T>
constexpr T min_gain_threshold = 0.01;

template <FloatingPt T>
constexpr T two_pi = std::numbers::pi_v<T> * 2;

/// @brief This concept enforces the use of unsigned integer values with a maximum size of 8 Byte (64 Bit).
/// @tparam T: Type to constrain.
template <typename T>
concept UnsignedL64 = std::is_unsigned_v<T> && sizeof(T) <= 8;

/// @brief Further constraint for the BoundedDegTwo and BoundedPowTwo types, so that the correct maximum can always
/// be assigned.
/// UnsignedGE16 = Spectral Unsigned Greater or Equal to 16 Byte.
/// @tparam T: Type to constrain.
template <typename T>
concept UnsignedGE16 = std::is_unsigned_v<T> && sizeof(T) <= 8 && sizeof(T) > 1;

/// @brief Check if the degree to calculate is in the valid range of this plugin.
/// @tparam T: Type of the degree to check.
/// @param degree_to_check Degree to check.
/// @return bool
template <UnsignedL64 T>
constexpr bool is_bounded_degree(const T degree_to_check) noexcept
{
    return degree_to_check <= max_pow_two_degree && degree_to_check >= min_pow_two_degree;
}

/// @brief Compile- or runtime check if a given number of samples is in the valid range of this plugin.
/// @tparam T: Type of the number of samples to check.
/// @param no_of_samples: The number of samples to check.
/// @return bool
template <UnsignedL64 T>
constexpr bool is_bounded_no_of_samples(T no_of_samples) noexcept
{
    return no_of_samples >= min_num_of_samples && no_of_samples <= max_num_of_samples;
}

/// @brief Checks the range, as well as that the number is actually a power of two.
/// @tparam T: Type of the number to check.
/// @param number: The number to check.
/// @return bool
template <UnsignedL64 T>
constexpr bool is_bounded_pow_two(const T number) noexcept
{
    // return is_bounded_no_of_samples(number) && is_pow_two(number);
    return is_bounded_no_of_samples(number) && std::has_single_bit(number);
}

/// @brief  Determine the value of a degree of a power of two.
/// The maximum value can't be greater than the maximum representable by the given type and at max 2^63 so
/// no overflow is possible (as opposed to the regular << operator).
/// @tparam T: Type of the degree (will also be the type of the returned value).
/// @param degree: The degree from which you want to know the corresponding power of two.
/// @return Value of type T
template <UnsignedL64 T>
constexpr T pow_two_value_of_degree(const T degree) noexcept
{
    // make sure the one to shift left has the same type as i_degree.
    constexpr T correct_one = 1;
    // safety check to prevent overflows.
    if (degree >= sizeof(T) * 8)
    {
        return (correct_one << sizeof(T) * 8 - 1);
    }
    return correct_one << degree;
}

/// @brief Alias for an array that contains pairs of indices and values.
/// Intended for the bin numbers (frequencies) and the respective magnitudes of a fourier transformed signal.
/// @tparam T: Type of the magnitudes (typaclly float or double).
/// @tparam N_SAMPLES: Number of samples that this array contains (typically half the size of the original FFT array).
/// Has to be a power of two!
template <FloatingPt T, size_t N_SAMPLES>
    requires(is_bounded_pow_two(N_SAMPLES))
using BinMagArr = std::array<std::pair<size_t, T>, N_SAMPLES>;

/// @brief Alias for an array containing complex numbers (basically just for conveniance) will contain values of FFT.
/// @tparam T: Type of the complex numbers.
/// @tparam N_SAMPLES: Number of samples, has to be a power of two.
template <FloatingPt T, size_t N_SAMPLES>
    requires(is_bounded_pow_two(N_SAMPLES))
using ComplexArr = std::array<std::complex<T>, N_SAMPLES>;

/// @brief This struct is mainly used as a security check to be sure to initialize values according to a real
/// power of two that is in the valid range of the samples used by this plugin. To assure that the maximum is always
/// a valid value, the type given has to have at least 16 Bit.
/// This is a compile-time check and NOT intended to be an object.
/// @tparam T: Type of the passed power of two.
/// @tparam POT: The value of the passed power of two.
template <UnsignedGE16 T, T POT>
    requires(is_bounded_pow_two(POT))
struct BoundedPowTwo
{
    BoundedPowTwo() = delete;
    static constexpr T value = POT;
};

/// @brief For conveniance you can access the value directly like in the c++ typic implementations.
template <UnsignedGE16 T, T POT>
constexpr T BoundedPowTwo_v = BoundedPowTwo<T, POT>::value;

/// @brief Like BoundedPowTwo but you specify a degree instead of a direct value. The rule with at least 16 Bit also
/// applies to this struct!
/// @tparam T: Type of the passed degree.
/// @tparam DEG: The value of the passed degree.
template <UnsignedGE16 T, uint8_t DEG>
    requires(is_bounded_degree(DEG))
struct BoundedDegTwo
{
    BoundedDegTwo() = delete;
    static constexpr uint8_t degree = DEG;
    static constexpr T value = pow_two_value_of_degree<T>(DEG);
};

/// @brief Access the actual number of samples that resulted from the given degree.
template <UnsignedGE16 T, uint8_t deg>
constexpr T BoundedDegTwo_v = BoundedDegTwo<T, deg>::value;

/// @brief Like clip_to_lower_pow_two but only for valid plugin values. To guarantee that the max value can be assigned,
/// a passed type has to be at least 16 bytes large!
/// @tparam T: Type of the value to clip
/// @param i_value: The value to clip
/// @return Clipped value of type T
template <UnsignedGE16 T>
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
    // explicit type shoudn't be needed since i_value is of type T but just to be sure...
    return std::bit_floor<T>(i_value);
}

/// @brief Get the degree of a power of two value.
/// @tparam T: Type of the degree that returns on success, by default uint8_t since degrees larger than 64 are not
/// possible with UnsignedL64.
/// @tparam V: Type of the value to check, defaultet to uint64_t to have the max resolution available if not statet
/// otherwise.
/// @param power_to_calculate: The power of two from which you want to know the corresponding degree. If the value
/// passed is not a power of two, it will be clipped to the closest lower power of two.
/// @return The degree of type T, corresponding to the passed (and maybe rounded) number
template <UnsignedL64 T = uint8_t, UnsignedL64 V = uint64_t>
constexpr T degree_of_pow_two_value(const V power_to_calculate) noexcept
{
    V guaranteed_pot = power_to_calculate;
    if (!std::has_single_bit(power_to_calculate))
    {
        guaranteed_pot = std::bit_floor(power_to_calculate);
    }
    constexpr V correct_sized_one = 1;
    T first_occurance = 0;
    while ((correct_sized_one << first_occurance) != guaranteed_pot)
    {
        ++first_occurance;
    }
    return first_occurance;
}

} // namespace LBTS::Spectral
