/**
 * Author: Lucas Scheidt
 * Date: 20.10.24
 *
 * Description: Implementation of a circular buffer that is used within the plugin to read and write from the passed
 * buffer chunks.
 */

#pragma once
#include <ranges>
#include "SpctDomainSpecific.h"
#include "SpctWavetables.h"

namespace LBTS::Spectral
{
/// Forward declaration of BufferManager to be able to use it as friend. Not thaaat beautiful but C++ forces you to
/// do such things, so be it.
template <FloatingPt T, size_t BUFFER_SIZE>
    requires(is_bounded_pow_two(BUFFER_SIZE))
class BufferManager;

/// @brief This is a circular buffer from which two instances will be used.
/// 1. as input to the FFT
/// 2. as output buffer from which the result will be read
/// One critical question remaining is do we create several instances of SampleBuffer (for different sizes)
/// or should the template be erased and the whole thing be dynamic.
template <FloatingPt T, size_t MAX_BUFFER_SIZE = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(MAX_BUFFER_SIZE))
struct CircularSampleBuffer
{
    using type = T;
    CircularSampleBuffer() = default;
    // explicit CircularSampleBuffer(const size_t i_range) { resize_valid_range(i_range); }
    ~CircularSampleBuffer() = default;

    /// @brief as the name says, note that only values in the active valid range get cleared!
    void reset_buffers() noexcept
    {
        m_in_array.fill(0);
        m_out_array.fill(0);
        m_index = 0;
    }

    void reset_out_buffer() noexcept
    {
        m_out_array.fill(0);
    }

    /// @brief shove one value in the current index position.
    void fill_input(const T t_value) noexcept { m_in_array[m_index] = t_value; }

    void copy_to_output() noexcept
    {
        // unfortunately apple clang does not support ranges::zip... for whatever fucking reason!
        auto windowed_in = std::views::iota(static_cast<size_t>(0), m_in_array.size()) |
                           std::views::transform([this](size_t ndx) { return m_in_array[ndx] * m_window[ndx]; });
        std::ranges::copy(windowed_in, m_out_array.begin());
        // for debug purposes just copying the in array would be the commented line beyond.
        // std::ranges::copy(m_in_array, m_out_array.begin());
    }

    /// @brief advancing by one, this happens synchronousely for both buffers.
    bool advance() noexcept
    {
        ++m_index;
        // transformation needs to be done when wrapping is needed.
        // COLA for Hamming = 0.5
        const bool do_transformation = m_index == m_view_size;
        // const bool do_transformation = (m_index & ~(m_view_size  >> 1)) == (m_view_size >> 1) - 1;
        // mask with the flipped view size
        // m_index & ~m_view_size
        // 01101   & ~(10000) = 01101 & 01111 = 01101
        // 10000   & ~(10000) = 10000 & 01111 = 00000
        m_index &= ~MAX_BUFFER_SIZE;
        return do_transformation;
    }

    [[nodiscard]] size_t current_index() const noexcept { return m_index; }

    /// @note thought back and forth and came to the conclusion, that I preferred having a friend that knows what to
    /// do with the internal arrays than to allow reference getters for them (or make them public).
    /// That way access is limited and safety is increased. Only downside is the forward declaration...
    friend BufferManager<T, MAX_BUFFER_SIZE>;

  private:
    size_t m_index{0};
    size_t m_view_size{MAX_BUFFER_SIZE};
    ComplexArr<T, MAX_BUFFER_SIZE> m_in_array{0};
    ComplexArr<T, MAX_BUFFER_SIZE> m_out_array{0};
    // Hamming with no overlap sounds best.
    //     const VonHannWindow<T, MAX_BUFFER_SIZE> m_window{};
    const HammingWindow<T, MAX_BUFFER_SIZE> m_window{};
};

} // namespace LBTS::Spectral
