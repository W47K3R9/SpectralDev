/**
 * Author: Lucas Scheidt
 * Date: 28.10.2024
 *
 * Purpose: This file contains domain specific declarations of concepts and types needed for the audio plugin to
 *  implement.
 */

// ReSharper disable CppAccessSpecifierWithNoDeclarations
// ^^^^^^^^^ becaues sometimes
// private: members
// private: functions
// is just more readable!
#pragma once

#include <algorithm>
#include <span>

namespace LBTS::Spectral
{
/**
 * @section
 * 1. Definition of constants regarding the maximum processing capability of the plugins in terms of samples.
 * These are not explicitely checked, but have been tested in the SpctPowTwoTest.h file, just to be sure to avoid
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

/// @brief plugin specific constants
constexpr unsigned min_pow_two_degree = 4;
constexpr unsigned max_pow_two_degree = 14;
constexpr unsigned min_num_of_samples = 16;
constexpr unsigned max_num_of_samples = 16384;

/// @brief check if the degree to calculate is in an appropriate range (2^14 = 16384 -> actual max)
template <typename T = unsigned>
    requires(std::is_unsigned_v<T>)
constexpr bool is_bounded_degree(const T i_degree)
{
    return i_degree <= max_pow_two_degree && i_degree >= min_pow_two_degree;
}
/// @brief compile- or runtime check if a given number of samples is in the valid range of this plugin.
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr bool is_bounded_no_of_samples(T i_samples)
{
    return i_samples >= min_num_of_samples && i_samples <= max_num_of_samples;
}
/// @brief checks the range, as well as that the number is actually a power of two.
template <typename T = unsigned>
    requires(std::is_unsigned_v<T>)
constexpr bool is_bounded_pow_two(const T i_number)
{
    return is_bounded_no_of_samples(i_number) && (i_number & (i_number - 1)) == 0;
}
/// @brief recursive funcution to determine the value of a degree of a power of two can be used at compile- or runtime.
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr T pow_two_value_of_degree(T i_degree)
{
    if (i_degree == 0)
    {
        return 1;
    }
    if (i_degree == 1)
    {
        return 2;
    }
    return pow_two_value_of_degree(i_degree - 1) * 2;
}

/// @brief This struct is mainly used as a security check to be sure to initialize values according to a real
/// power of two that is in the valid range of the samples used by this plugin.
/// This is a compile-time check and NOT intended to be an object.
template <typename T, T pot>
    requires(is_bounded_pow_two(pot))
struct BoundedPowTwo
{
    BoundedPowTwo() = delete;
    static constexpr T value = pot;
};
/// @brief and for conveniance you can access the value directly.
template <typename T, T pot>
constexpr T BoundedPowTwo_v = BoundedPowTwo<T, pot>::value;

/// @brief clamp the given number to the closest lower power of two (except for 0 where the 2^0 = 1 is returned).
/// Example:
/// input:  19 (0b0000 1011)
/// output: 16 (0b0000 1000)
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr T clip_to_lower_pow_two(T i_value)
{
    if (i_value == 0)
    {
        return 1;
    }
    constexpr decltype(i_value) correct_sized_one = 1;
    constexpr auto size_of_value = correct_sized_one << sizeof(T) * 8 - 1;
    // won't be greater than 64 (ull)
    unsigned first_occurance = 0;
    while (!(static_cast<unsigned long>(i_value << first_occurance) & size_of_value))
    {
        ++first_occurance;
    }
    return correct_sized_one << sizeof(T) * 8 - 1 - first_occurance;
}
/// @brief like above but only for valid plugin values.
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr T clip_to_lower_bounded_pow_two(T i_value)
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

/// @brief like BoundedPowTwo but you specify a degree instead of a direct value. If this struct is used at compiletime
/// no overhead for the recursive function is needed during runtime.
template <unsigned deg, typename T = unsigned>
    requires(is_bounded_degree(deg))
struct BoundedDegTwo
{
    BoundedDegTwo() = delete;
    static constexpr unsigned degree = deg;
    static constexpr T value = pow_two_value_of_degree<T>(deg);
};
template <unsigned deg, typename T = unsigned>
/// @brief access the actual number of samples that resulted from the given degree.
constexpr T BoundedDegTwo_v = BoundedDegTwo<deg, T>::value;

/// @brief This is a circular buffer from which two instances will be used. The 'CT' stands for Compile Time.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <typename T, size_t max_buffer_size = BoundedPowTwo_v<size_t, max_num_of_samples>>
    requires(is_bounded_pow_two(max_buffer_size))
struct CircularSampleBuffer
{
    using type = T;
    CircularSampleBuffer() = default;
    explicit CircularSampleBuffer(const size_t i_range) { resize_valid_range(i_range); }
    ~CircularSampleBuffer() = default;
    void resize_valid_range(const size_t i_range) noexcept
    {
        // first range check needed since the max_buffer_size doesn't have to be the max number of samples the plugin
        // allows (which is checked by is_power_of_two).
        if (i_range >= max_buffer_size)
        {
            return;
        }
        // valid range guaranteed by clipping to valid range.
        if (!is_bounded_pow_two(i_range))
        {
            m_view_size = clip_to_lower_bounded_pow_two(i_range);
        }
        else
        {
            m_view_size = i_range;
        }
    }
    void reset_buffers() noexcept
    {
        /// @note not the most efficient approach but memset may be optimized away and since this get's only called
        /// when resizing the buffer that shouldn't be a problem, also only the needed values are being reset.
        for (auto& el : std::span(m_in_array, m_view_size))
        {
            el = 0;
        }
        for (auto& el : std::span(m_out_array, m_view_size))
        {
            el = 0;
        }
        m_index = 0;
    }
    void fill_input(const T t_value) noexcept { m_in_array[m_index] = t_value; }
    T receive_output() const noexcept { return m_out_array[m_index]; }
    /// @note advancing happens synchronousely for both buffers.
    bool advance() noexcept
    {
        ++m_index;
        // transformation needs to be done when wrapping is needed.
        const bool do_transformation = m_index == m_view_size;
        m_index &= m_view_size - 1;
        return do_transformation;
    }
    [[nodiscard]] size_t current_index() const noexcept { return m_index; }
    [[nodiscard]] size_t size() const noexcept { return m_view_size; }
    /// @brief used to be filled by the FFT, this is *DANGEROUS* since no range checks are done.
    /// The calling function MUST assure to provide an array that is AT LEAST THE SIZE OF THE CURRENT VIEW SIZE!
    void fill_output_unsafe(T* t_array_to_be_copied)
    {
        std::copy(t_array_to_be_copied, t_array_to_be_copied + m_view_size, m_out_array);
    }

  private:
    size_t m_index{0};
    size_t m_view_size{BoundedPowTwo_v<size_t, max_buffer_size>};
    T m_in_array[BoundedPowTwo_v<size_t, max_buffer_size>]{0};
    T m_out_array[BoundedPowTwo_v<size_t, max_buffer_size>]{0};
};

/// @brief Allows a view on a C style array while providing begin() and end() T* iterators.
/// Note that every parameter needs to be known at compile time so this class is not suited for arrays of changing size.
template <typename T, size_t slice_size, size_t original_size = 16384>
    requires(is_bounded_pow_two(slice_size) && is_bounded_pow_two(original_size) && original_size >= slice_size)
struct StaticSampleArrayView
{
  public:
    using type = T;
    explicit StaticSampleArrayView(T* t_array_to_pass) : m_array_pointer{t_array_to_pass} {}
    ~StaticSampleArrayView() = default;
    /// @note Unsafe because there is NO range check on t_slice!
    void write_to_slice_unsafe(T* t_slice)
    {
        /// @todo in here loop unrolling may be helpful.
        for (unsigned i = 0; i < slice_size; ++i)
        {
            m_array_pointer[i + m_slice_position] = t_slice[i];
        }
    }
    void write_to_slice(std::array<T, slice_size>& t_slice)
    {
        std::copy(t_slice.begin(), t_slice.end(), this->begin());
    }
    /// @brief Hops from one slice to the next without overflowing.
    void advance() noexcept
    {
        m_slice_position += slice_size;
        m_slice_position &= original_size - 1;
    }
    /// @brief Bracket implementation with range check in sense of wrapping around max value.
    /// This will prevent a bad access BUT unexpected results may be returned if you try to access a value outside
    /// the slice size range.
    T operator[](const size_t t_position_to_read) { return m_array_pointer[t_position_to_read & slice_size - 1]; }
    T operator[](const size_t t_position_to_read) const { return m_array_pointer[t_position_to_read & slice_size - 1]; }
    T* begin() const noexcept { return m_array_pointer + m_slice_position; }
    T* end() const noexcept { return m_array_pointer + m_slice_position + slice_size; }
    [[nodiscard]] unsigned size() const noexcept { return slice_size; }

  private:
    size_t m_slice_position = 0;
    T* m_array_pointer;
};
} // namespace LBTS::Spectral
