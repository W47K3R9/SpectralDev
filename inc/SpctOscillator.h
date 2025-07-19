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
#include <mutex>
#include <valarray>

namespace LBTS::Spectral
{



// static constexpr size_t clamp_index(size_t* object_index_ptr)
// {
//     // 0111 -> 0111
//     // 1000 -> 0111
//     const uint8_t bit_1 =
//    return 0;
// }

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
          m_wt_ptr(other.m_wt_ptr),
          m_freq_glide_arr(other.m_freq_glide_arr),
          m_amp_glide_arr(other.m_amp_glide_arr)
    {
        other.m_wt_ptr = nullptr;
    };

    WTOscillator& operator=(WTOscillator&& other) noexcept
    {
        m_table_index = other.m_table_index;
        m_sampling_freq = other.m_sampling_freq;
        m_nyquist_freq = m_sampling_freq / 2.0;
        m_inv_sampling_freq = 1.0 / m_sampling_freq;
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
        if (const auto index_incr = m_freq_glide_arr[m_freq_glide_index];
            (m_table_index += index_incr) >= (m_internal_size))
        {
            m_table_index -= m_internal_size;
        }

        const T sample = output * m_amp_glide_arr[m_amp_glide_index];

        m_amp_glide_index = std::clamp<size_t>(++m_amp_glide_index, 0, GLIDE_SIZE - 1);
        m_freq_glide_index = std::clamp<size_t>(++m_freq_glide_index, 0, GLIDE_SIZE - 1);
        return sample;
    }

    /// @brief Resets the sampling frequency as specified and the table index to 0.
    /// @param sampling_freq The wanted sampling frequency.
    void reset(const double sampling_freq) noexcept
    {
        m_table_index = 0.0f;
        m_sampling_freq = sampling_freq;
        m_nyquist_freq = sampling_freq / 2.0;
        m_inv_sampling_freq = 1.0 / sampling_freq;
    }

    /// @brief This will set the increment rate inside the wavetable.
    /// @param to_freq The oscillator will output it's waveform with this frequency (in Hz).
    void tune(T to_freq) noexcept
    {
        // m_index_increment_from.store(m_index_increment);
        // be sure not to tune above nyquist!
        to_freq = std::clamp<T>(to_freq, 0, m_nyquist_freq);
        // increment = N_WT * f0 / fs
        // steps from one sample to the next.
        const auto end_val = m_internal_size * to_freq * m_inv_sampling_freq;
        const auto start_val = m_freq_glide_arr.front();
        uint8_t current_index = 0;
        for (auto& val : m_freq_glide_arr)
        {
            ++current_index;
            const auto fract = static_cast<float>(current_index) / static_cast<float>(GLIDE_SIZE);
            val = start_val + fract * (end_val - start_val);
        }
        m_freq_glide_index = 0;
    }

    /// @brief Set the amplitude of an oscillator
    /// @param amplitude value of the amplitude
    void set_amplitude(T amplitude) noexcept
    {
        const auto end_val = amplitude;
        const auto start_val = m_amp_glide_arr.front();
        uint8_t current_index = 0;
        for (auto& val : m_amp_glide_arr)
        {
            ++current_index;
            const auto fract = static_cast<float>(current_index) / static_cast<float>(GLIDE_SIZE);
            val = start_val + fract * (end_val - start_val);
        }
        m_amp_glide_index = 0;
    }


    /// @brief Change the look up table.
    /// @param wt_ptr A pointer to the wanted lookup table.
    void change_waveform(const WaveTable<T, WT_SIZE>* wt_ptr) { m_wt_ptr = wt_ptr; }

  private:
    // float is precise enough for interpolation between indices
    float m_table_index = 0.0f;

    double m_sampling_freq = 44100.0;
    double m_nyquist_freq = m_sampling_freq / 2.0;
    double m_inv_sampling_freq = 1.0 / m_sampling_freq;
    const WaveTable<T, WT_SIZE>* m_wt_ptr = nullptr;
    static constexpr size_t m_internal_size = WT_SIZE - 1;

    static constexpr auto GLIDE_SIZE = BoundedPowTwo_v<size_t, 8>;
    std::array<float, GLIDE_SIZE> m_freq_glide_arr{};
    std::atomic<size_t> m_freq_glide_index = 0;
    std::array<T, GLIDE_SIZE> m_amp_glide_arr{};
    std::atomic<size_t> m_amp_glide_index = 0;

};
} // namespace LBTS::Spectral
