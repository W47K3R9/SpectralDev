/**
 * Author: Lucas Scheidt
 * Date: 20.10.24
 *
 * Description: Implementation of a circular buffer that is used within the plugin to read and write from the passed
 * buffer chunks.
 */

#pragma once
#include "SpctDomainSpecific.h"
#include "SpctWavetables.h"
#include <algorithm>
#include <ranges>

namespace LBTS::Spectral
{
/// Forward declaration of CalculationEngine to be able to use it as friend. Not thaaat beautiful but C++ forces you to
/// do such things, so be it.
template <FloatingPt T, size_t BUFFER_SIZE>
    requires(is_bounded_pow_two(BUFFER_SIZE))
class CalculationEngine;

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

    void clear_arrays() noexcept
    {
        m_in_array.fill(0);
        m_out_array.fill(0);
        m_ringbuffer_index = 0;
    }

    /// @brief shove one value in the current index position.
    /// In regular FFT it would make sense to window the input but a rectangular window sounds best in this scenario.
    // void fill_input(const T t_value) noexcept { m_in_array[m_ringbuffer_index] = t_value; }
    void fill_input(const T t_value) noexcept { m_in_array[m_ringbuffer_index] = t_value * m_window_compensation; }

    void copy_to_output() noexcept
    {
        /// @todo use views::zip as soon as available in apple clang.
        auto windowed_in = std::views::iota(static_cast<size_t>(0), m_in_array.size()) |
                           std::views::transform([this](size_t ndx) { return m_in_array[ndx] * m_window[ndx]; });
        std::ranges::copy(windowed_in, m_out_array.begin());
        // only for debugging purposes
        /// std::ranges::copy(m_in_array, m_out_array.begin());
    }

    /// @brief advancing by one, this happens synchronousely for both buffers.
    bool advance() noexcept
    {
        ++m_ringbuffer_index;
        const bool do_transformation = m_ringbuffer_index == VIEW_SIZE;
        // mask with the flipped view size
        // m_ringbuffer_index & ~m_view_size
        // 01101   & ~(10000) = 01101 & 01111 = 01101
        // 10000   & ~(10000) = 10000 & 01111 = 00000
        m_ringbuffer_index &= ~MAX_BUFFER_SIZE;
        return do_transformation;
    }

    [[nodiscard]] size_t current_index() const noexcept { return m_ringbuffer_index; }

    /// @note thought back and forth and came to the conclusion, that I preferred having a friend that knows what to
    /// do with the internal arrays than to allow reference getters for them (or make them public).
    /// That way access is limited and safety is increased. Only downside is the forward declaration...
    friend CalculationEngine<T, MAX_BUFFER_SIZE>;

  private:
    static constexpr size_t VIEW_SIZE = MAX_BUFFER_SIZE / 2;
    size_t m_ringbuffer_index{0};

    std::array<T, MAX_BUFFER_SIZE> m_in_array{0};
    ComplexArr<T, MAX_BUFFER_SIZE> m_out_array{0};

    T m_window_compensation = static_cast<T>(1.2);
    VonHannWindow<T, MAX_BUFFER_SIZE> m_window{};
    // HammingWindow<T, MAX_BUFFER_SIZE> m_window{};
    // const BartlettWindow<T, MAX_BUFFER_SIZE> m_window{};
};

} // namespace LBTS::Spectral
