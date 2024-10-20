/**
 * Author: Lucas Scheidt
 * Date: 20.10.24
 *
 * Description: Implementation of a circular buffer that is used within the plugin to read and write from the passed
 * buffer chunks.
 */

#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{
/// @brief This is a circular buffer from which two instances will be used. The 'CT' stands for Compile Time.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <typename T, size_t max_buffer_size = BoundedDegTwo_v<max_pow_two_degree, size_t>>
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
    size_t m_view_size{BoundedDegTwo_v<max_pow_two_degree, size_t>};
    T m_in_array[BoundedDegTwo_v<max_pow_two_degree, size_t>]{0};
    T m_out_array[BoundedDegTwo_v<max_pow_two_degree, size_t>]{0};
};

} // namespace LBTS::Spectral