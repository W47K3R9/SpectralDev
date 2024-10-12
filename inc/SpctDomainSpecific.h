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
 * 2. Implementation of several compile- or runtime checks if the specified limits are respected and if a number is
 * actually a power of two.
 * The sampling rate of a project can change at runtime. It's not recommended though to use an arbitrary large amount of
 * samples (processing power is not infinite!). For now, I'll set the maximum to 500ms with 48 kHz.
 * Furthermore, a sufficient amount of samples is needed for an accurate transformation.
 * Frequency resolution = sampling freq (fs) / number of samples (n)
 * 16 samples leads to a miserable 3 kHz resolution with 48 kHz fs but let's see how it sounds :D
 */
constexpr unsigned min_pow_two_degree = 4;
constexpr unsigned max_pow_two_degree = 14;
constexpr unsigned min_num_of_samples = 16;
constexpr unsigned max_num_of_samples = 16384;
/// @brief check if the degree to calculate is in an appropriate range (2^14 = 16384 -> actual max)
template <typename T = unsigned>
    requires(std::is_unsigned_v<T>)
constexpr bool degree_is_in_range(const T i_degree)
{
    return i_degree <= max_pow_two_degree && i_degree >= min_pow_two_degree;
}

/// @brief compile- or runtime check if a given number of samples is in the valid range of this plugin.
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr bool n_of_samples_in_valid_range(T i_samples)
{
    return i_samples >= min_num_of_samples && i_samples <= max_num_of_samples;
}

/// @brief checks the range, as well as that the number is actually a power of two.
template <typename T = unsigned>
    requires(std::is_unsigned_v<T>)
constexpr bool is_power_of_two(const T i_number)
{
    return n_of_samples_in_valid_range(i_number) && (i_number & (i_number - 1)) == 0;
}

/// @brief This struct is mainly used as a security check for being sure to initialize values according to a real
/// power of two. This is a compile-time check and NOT intended to be an object.
template <typename T, T pot>
    requires(is_power_of_two(pot))
struct PowTwo
{
    PowTwo() = delete;
    static constexpr T value = pot;
};
/// And for conveniance you can access the value directly.
template <typename T, T pot>
constexpr T PowTwo_v = PowTwo<T, pot>::value;

/// @brief clamp the given number to the closest lower power of two (except for 0 where the 2^0 = 1 is returned).
/// Example:
/// input:  19 (0b0000 1011)
/// output: 16 (0b0000 1000)
template <typename T>
    requires(std::is_unsigned_v<T>)
constexpr T lower_clip_to_power_of_two(T i_value)
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
constexpr T lower_clip_to_valid_power_of_two(T i_value)
{
    if (i_value <= min_num_of_samples)
    {
        return PowTwo_v<T, min_num_of_samples>;
    }
    if (i_value >= max_num_of_samples)
    {
        return PowTwo_v<T, max_num_of_samples>;
    }
    return lower_clip_to_power_of_two(i_value);
}

/// @brief This struct is mainly used as a security check for being sure to initialize values according to a real
///     power of two. This is a compile- or runtime check and NOT intended to be an object.
template <unsigned pot>
    requires(is_power_of_two(pot))
struct POTSamples
{
    POTSamples() = delete;
    static constexpr unsigned value = pot;
};


/// @brief little recursive funcution to determine the value of a degree of a power of two.
template <typename T>
constexpr T pot_value_of_degree(T i_degree)
{
    if (i_degree == 1)
    {
        return 2;
    }
    return pot_value_of_degree_ct(i_degree - 1) * 2;
}

/// @brief maybe in some scenarios it's more conveniant to specify a degree and derive the power of two value
/// from the degree.
template <unsigned deg, typename T = unsigned>
    requires(degree_is_in_range(deg))
struct DegTwo
{
    DegTwo() = delete;
    static constexpr unsigned degree = deg;
    static constexpr T value = pot_value_of_degree<T>(deg);
};

/// @brief This is a circular buffer from which two instances will be used. The 'CT' stands for Compile Time.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <typename T, size_t max_buffer_size = PowTwo_v<size_t, max_num_of_samples>>
    requires(is_power_of_two(max_buffer_size))
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
        if (!is_power_of_two(i_range))
        {
            m_view_size = lower_clip_to_valid_power_of_two(i_range);
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
    size_t m_view_size{PowTwo_v<size_t, max_buffer_size>};
    T m_in_array[PowTwo_v<size_t, max_buffer_size>]{0};
    T m_out_array[PowTwo_v<size_t, max_buffer_size>]{0};
};

/// @brief Allows a view on a C style array while providing begin() and end() T* iterators.
/// Note that every parameter needs to be known at compile time so this class is not suited for arrays of changing size.
template <typename T, size_t slice_size, size_t original_size = 16384>
    requires(is_power_of_two(slice_size) && is_power_of_two(original_size) && original_size >= slice_size)
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
