/**
 * Author: Lucas Scheidt
 * Date: 20.10.24
 *
 * Description: Implementation of a circular buffer that is used within the plugin to read and write from the passed
 * buffer chunks.
 */

#pragma once
#include "SpctDomainSpecific.h"
#include <array>
#include <complex>

namespace LBTS::Spectral
{
/// @brief This is a circular buffer from which two instances will be used.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <typename T, size_t MAX_BUFFER_SIZE = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(MAX_BUFFER_SIZE))
struct CircularSampleBuffer
{
    using type = T;
    CircularSampleBuffer() = default;
    // explicit CircularSampleBuffer(const size_t i_range) { resize_valid_range(i_range); }
    ~CircularSampleBuffer() = default;

    /// @brief resizes the internal range the buffer works with (I know const is unnecessary here but I like it for
    /// readability).
    /// ON HOLD!
    void resize_valid_range(const size_t i_range) noexcept;

    /// @brief as the name says, note that only values in the active valid range get cleared!
    void reset_buffers() noexcept;

    /// @brief shove one value in the current index position.
    void fill_input(const T t_value) noexcept { m_in_array[m_index] = t_value; }

    /// @brief get one value from the current index position.
    T receive_output() const noexcept { return m_out_array[m_index]; }

    /// @brief advancing by one, this happens synchronousely for both buffers.
    bool advance() noexcept;

    [[nodiscard]] size_t current_index() const noexcept { return m_index; }

    [[nodiscard]] size_t size() const noexcept { return m_view_size; }

    /// @brief used to be filled by the FFT, this is *DANGEROUS* since no range checks are done.
    /// The calling function MUST assure to provide an array that is AT LEAST THE SIZE OF THE CURRENT VIEW SIZE!
    void fill_output_unsafe(T* t_array_to_be_copied)
    {
        std::copy(t_array_to_be_copied, t_array_to_be_copied + m_view_size, m_out_array.begin());
    }

    /// @brief pass the filled input array by reference (to the FFT calculation)
    std::array<std::complex<T>, MAX_BUFFER_SIZE>& get_in_array_ref() noexcept { return m_in_array; }

  private:
    size_t m_index{0};
    size_t m_view_size{MAX_BUFFER_SIZE};
    std::array<std::complex<T>, MAX_BUFFER_SIZE> m_in_array{0};
    std::array<T, MAX_BUFFER_SIZE> m_out_array{0};
};

template <typename T, size_t MAX_BUFFER_SIZE>
    requires(is_bounded_pow_two(MAX_BUFFER_SIZE))
void CircularSampleBuffer<T, MAX_BUFFER_SIZE>::reset_buffers() noexcept
{
    m_in_array.fill(0);
    m_out_array.fill(0);
    m_index = 0;

    // DEPRECATED! May be a liiiittle faster than the implementation above but may be never needed.
    // for (auto& el : std::span(m_in_array.begin(), m_view_size))
    // {
    //     el = 0;
    // }
    // for (auto& el : std::span(m_out_array.begin(), m_view_size))
    // {
    //     el = 0;
    // }
}

template <typename T, size_t MAX_BUFFER_SIZE>
    requires(is_bounded_pow_two(MAX_BUFFER_SIZE))
bool CircularSampleBuffer<T, MAX_BUFFER_SIZE>::advance() noexcept
{
    ++m_index;
    // transformation needs to be done when wrapping is needed.
    const bool do_transformation = m_index == m_view_size;
    m_index &= m_view_size - 1;
    return do_transformation;
}

// ON HOLD!
template <typename T, size_t MAX_BUFFER_SIZE>
    requires(is_bounded_pow_two(MAX_BUFFER_SIZE))
void CircularSampleBuffer<T, MAX_BUFFER_SIZE>::resize_valid_range(const size_t i_range) noexcept
{
    // first range check needed since the MAX_BUFFER_SIZE doesn't have to be the max number of samples the plugin
    // allows (which is checked by is_power_of_two).
    if (i_range >= MAX_BUFFER_SIZE)
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
} // namespace LBTS::Spectral