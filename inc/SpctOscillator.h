/**
 * Author: Lucas Scheidt
 * Date: 27.12.24
 *
 * Description: Functions for oscillators. First implementation will be sine, tri, square and ramp up.
 * Oscillators will be implemented as wavetable oscillators because... Well because I can and it's computationally not
 * too expensive.
 */

#pragma once
#include "SpctDomainSpecific.h"
#include "SpctWavetables.h"
#include <atomic>

namespace LBTS::Spectral
{

/// @brief A single wavetable oscillator.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
class WTOscillator
{
  public:
    /// @brief Needed for initial creation.
    WTOscillator() = default;

    /// @brief Concrete and valid creation.
    /// @param sampling_freq Specified by DAW environment.
    /// @param wt_ptr Pointer to a wavetable. If nullptr is given the Oscillator will only output zeros.
    WTOscillator(const double sampling_freq, const WaveTable<T, WT_SIZE>* wt_ptr)
        : m_sampling_freq{sampling_freq},
          m_inv_sampling_freq{1.0 / sampling_freq},
          m_wt_ptr{wt_ptr}
    {
        m_wt_ptr->equalize_end_and_begin();
    }

    WTOscillator(const WTOscillator&) noexcept = delete;
    WTOscillator& operator=(const WTOscillator&) = delete;
    /// @note Defaulted.
    WTOscillator(WTOscillator&& other) noexcept
        : m_table_index(other.m_table_index),
          m_sampling_freq(other.m_sampling_freq),
          m_wt_ptr(other.m_wt_ptr)
    {
        // m_index_increment_from.store(other.m_index_increment_from);
        m_index_increment.store(other.m_index_increment);
        m_amplitude.store(other.m_amplitude);
        other.m_wt_ptr = nullptr;
    };

    WTOscillator& operator=(WTOscillator&& other) noexcept
    {
        m_table_index = other.m_table_index;
        // m_index_increment_from.store(other.m_index_increment_from);
        m_index_increment.store(other.m_index_increment);
        m_sampling_freq = other.m_sampling_freq;
        m_nyquist_freq = m_sampling_freq / 2.0;
        m_inv_sampling_freq = 1.0 / m_sampling_freq;
        m_amplitude.store(other.m_amplitude);
        m_wt_ptr = other.m_wt_ptr;
        other.m_wt_ptr = nullptr;
        return *this;
    };

    /// @brief Defaulted since no dynamic resources were acquired.
    ~WTOscillator() = default;

    /// @brief Get the current output value of the oscillator and increase the phase to the next wavetable entry.
    /// @return The interpolated value of between two wavetable entries.
    /// @note Since this get's called in the main audio loop the wavetable pointer HAS TO BE VALID when calling this
    /// function! Checking this inside the function is too expensive.
    [[nodiscard]] T advance_and_receive_output() noexcept
    {
        // 1. determine the index as a whole number
        // 2. get the index above that number and wrap around if 1. is the last index of the WT
        // const auto current_index = static_cast<size_t>(m_table_index);
        // const size_t next_index = (current_index == WT_SIZE - 1) ? 0 : current_index + 1;

        // -> the wrap around for next_index to be 0 if current_index == WT_SIZE - 1 can be omitted if the oscillator
        // works with an internal size smaller by 1 than the wavetable.
        // Then the current m_table_index will wrap around if the increase by m_index_increment leads to a value greater
        // than WT_SIZE - 1. In order to still work properly the last index of the WT has to be the same as the first.

        // Example:
        // -> with conditional check the fractions will be between:
        // [0],[1]
        // [1],[2]
        // ...
        // [510],[511]
        // [511],[0]
        // [0],[1]
        // ... Everything works as expected, all fractions transition from one pair in the period to the next.
        //
        // -> without conditional check:
        // [0],[1]
        // [1],[2]
        // ...
        // [510],[511]
        // [0],[1]
        // ... Since m_table_index will get wrapped if it exceeds 510, the fraction between [511] and [0] is missing
        // Therefore the value of [511] should be the same as the value of [0]. Thus the wavetable is artificially
        // shortened although it still contains 512 values.

        const auto current_index = static_cast<size_t>(m_table_index);
        const size_t next_index = current_index + 1;
        // 3. get the values at these indices
        const T value_a = (*m_wt_ptr)[current_index];
        const T value_b = (*m_wt_ptr)[next_index];
        // 4. get the fraction offset and interpolate the output value
        const float fraction = m_table_index - static_cast<float>(current_index);
        const T output = value_a + (fraction * (value_b - value_a));
        // 5. wrap if needed
        if ((m_table_index += m_index_increment) >= (m_internal_size))
        {
            m_table_index -= m_internal_size;
        }
        return output * m_amplitude;
    }

    /// @brief Resets the sampling frequency as specified and the table index to 0.
    /// @param sampling_freq The wanted sampling frequency.
    void reset(const double sampling_freq) noexcept
    {
        m_amplitude = 0;
        m_table_index = 0;
        m_sampling_freq = sampling_freq;
        m_nyquist_freq = sampling_freq / 2.0;
        m_inv_sampling_freq = 1.0 / sampling_freq;
        m_glide_fraction = {0, 0};
        m_lower_limit = {0, 0};
        m_upper_limit = {0, 0};
    }

    /// @brief This will set the increment rate inside the wavetable as well as the oscillators amplitude.
    /// @param to_freq The oscillator will output it's waveform with this frequency (in Hz).
    /// @param amplitude value of the amplitude
    void tune_and_set_amp(T to_freq, const T amplitude) noexcept
    {
        // m_index_increment_from.store(m_index_increment);
        // be sure not to tune above nyquist!
        to_freq = std::clamp<T>(to_freq, 0, m_nyquist_freq);
        // increment = N_WT * f0 / fs
        // steps from one sample to the next.
        m_index_increment = m_internal_size * to_freq * m_inv_sampling_freq;
        m_amplitude.store(amplitude);
    }

    /// @brief Change the look up table.
    /// @param wt_ptr A pointer to the wanted lookup table.
    void change_waveform(const WaveTable<T, WT_SIZE>* wt_ptr) { m_wt_ptr = wt_ptr; }

  private:
    using FreqAmpPair = std::pair<std::atomic<T>, std::atomic<T>>;
    // float is precise enough for interpolation between indices
    float m_table_index = 0.0f;
    std::atomic<float> m_index_increment = 0.0f;

    /// @todo this should be a configurable parameter!
    std::atomic<T> m_amplitude = 0;

    double m_sampling_freq = 44100.0;
    double m_nyquist_freq = m_sampling_freq / 2.0;
    double m_inv_sampling_freq = 1.0 / m_sampling_freq;
    const WaveTable<T, WT_SIZE>* m_wt_ptr = nullptr;
    static constexpr size_t m_internal_size = WT_SIZE - 1;

    FreqAmpPair m_glide_fraction;
    FreqAmpPair m_upper_limit;
    FreqAmpPair m_lower_limit;
    T m_last_freq;
    T m_last_amp;

};
} // namespace LBTS::Spectral
