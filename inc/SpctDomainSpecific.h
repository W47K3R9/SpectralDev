/**
 * Author: Lucas Scheidt
 * Date: 28.10.2024
 *
 * Purpose: This file contains domain specific declarations of concepts and types needed for the audio plugin to
 *  implement.
 */

// ReSharper disable CppAccessSpecifierWithNoDeclarations
// ^^^^^^^^^ becaues sometimes private: members, private: functions is just more readable!
#pragma once

#include <algorithm>

namespace LBTS::Spectral
{
/// @note The sampling rate of a project can change at runtime.
///     It's not recommended to use an arbitrary large amount of samples nonetheless.
///     For now, I'll set the maximum to 500ms with 48 kHz. Furthermore, a sufficient amount of samples is needed for
///     an accurate transformation. Frequency resolution = sampling freq (fs) / number of samples (n)
///     16 samples leads to a miserable 3 kHz resolution with 48 kHz fs but let's see how it sounds :D
/// @todo Run performance test to see how long an FFT takes for different amounts of samples.
template <unsigned max_num_of_samples = 24000, unsigned min_num_of_samples = 16>
constexpr bool is_power_of_two(unsigned const number)
{
    return number <= max_num_of_samples && number >= min_num_of_samples && number != 0 && (number & (number - 1)) == 0;
}

/// @brief This struct is mainly used as a security check for being sure to initialize values according to a real
///     power of two. This is a compile-time check and NOT intended to be an object.
template <unsigned pot>
    requires(is_power_of_two(pot))
struct POTSamples
{
    POTSamples() = delete;
    static constexpr unsigned value = pot;
};

/// @brief This is a circular buffer from which two instances will be used. The 'CT' stands for Compile Time.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <typename T, size_t buffer_size>
struct CTSampleBuffer
{
    using type = T;
    CTSampleBuffer() = default;
    ~CTSampleBuffer() = default;
    void reset_buffers() noexcept
    {
        /// @note not the most efficient approach but memset may be optimized away...
        for(auto& el : m_in_array) {el = 0;}
        for(auto& el : m_out_array) {el = 0;}
        m_index = 0;
    }
    void fill_input(const T t_value) noexcept { m_in_array[m_index] = t_value; }
    T receive_output() const noexcept { return m_out_array[m_index]; }
    /// @note advancing happens synchronousely for both buffers.
    bool advance() noexcept
    {
        ++m_index;
        // transformation needs to be done when wrapping is needed.
        const bool do_transformation = m_index == buffer_size;
        m_index &= buffer_size - 1;
        return do_transformation;
    }
    [[nodiscard]] size_t current_index() const noexcept { return m_index; }
    [[nodiscard]] size_t size() const noexcept { return buffer_size; }
    /// @brief used to be filled by the FFT, this is DANGEROUS since no range checks are done.
    /// The calling function MUST assure to provide the EXACT CORRECT size!
    void fill_output(T* t_array_to_be_copied)
    {
        std::copy(t_array_to_be_copied, t_array_to_be_copied + buffer_size, m_out_array);
    }

  private:
    size_t m_index{0};
    T m_in_array[POTSamples<buffer_size>::value]{0};
    T m_out_array[POTSamples<buffer_size>::value]{0};
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
