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
    {}

    /// @note Deleted, no copies to be faster!
    WTOscillator(const WTOscillator&) noexcept = delete;
    /// @note Deleted, no copies to be faster!
    WTOscillator& operator=(const WTOscillator&) = delete;
    /// @note Defaulted.
    WTOscillator(WTOscillator&&) noexcept = default;
    /// @note Defaulted.
    WTOscillator& operator=(WTOscillator&&) noexcept = default;

    /// @brief Defaulted since no dynamic resources were acquired.
    ~WTOscillator() = default;

    /// @brief Get the current output value of the oscillator and increase the phase to the next wavetable entry.
    /// @return The interpolated value of between two wavetable entries.
    /// @note Since this get's called in the main audio loop the wavetable pointer HAS TO BE VALID when calling this
    /// function! Checking this inside the function is too expensive.
    T receive_output() noexcept;

    /// @brief Resets the sampling frequency as specified and the table index to 0.
    /// @param sampling_freq The wanted sampling frequency.
    void reset(const double sampling_freq) noexcept;

    /// @brief This will set the increment rate inside the wavetable.
    /// @param to_freq The oscillator will output it's waveform with this frequency (in Hz).
    void tune(T to_freq) noexcept;

    /// @brief Change the look up table.
    /// @param wt_ptr A pointer to the wanted lookup table.
    void change_waveform(const WaveTable<T, WT_SIZE>* wt_ptr) { m_wt_ptr = wt_ptr; }

  private:
    // float is precise enough for interpolation between indices
    float m_table_index = 0.0f;
    float m_index_increment = 0.0f;

    float m_index_increment_old = 0.0f;
    float m_glide_increment = 0.0f;
    float m_glide_fraction = 0.0001f;

    double m_sampling_freq = 44100.0;
    double m_nyquist_freq = m_sampling_freq / 2.0;
    double m_inv_sampling_freq = 1.0 / m_sampling_freq;
    const WaveTable<T, WT_SIZE>* m_wt_ptr = nullptr;
    bool m_valid_instantiation = false;
};

/// @brief Type definition for an array of wavetable oscillators.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
using OscArray = std::array<WTOscillator<T, WT_SIZE>, max_oscillators>;

/// @brief Object containing all oscillators that will resynthesize the FFT transformed input.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
/// @tparam FFT_SIZE Samples used for the fourier transformation.
template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
class ResynthOscs
{
  public:
    /// @brief Avoid instantiation with a wrong sampling frequency.
    ResynthOscs() = delete;

    /// @brief Correct instantiation with a valid sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    explicit ResynthOscs(const double sampling_freq);

    /// @note Deleted! Rule of zero.
    ResynthOscs(const ResynthOscs&) = delete;
    /// @note Deleted! Rule of zero.
    ResynthOscs& operator=(const ResynthOscs&) = delete;
    /// @note Deleted! Rule of zero.
    ResynthOscs(ResynthOscs&&) noexcept = delete;
    /// @note Deleted! Rule of zero.
    ResynthOscs& operator=(ResynthOscs&&) = delete;

    /// @brief Defaulted since no dynamic resources were acquired.
    ~ResynthOscs() = default;

    /// @brief Get the summed output of all playing oscillators.
    /// @param bin_mag_arr Array containing pairs of the frequency bin and it's asociated amplitude (needed for
    /// amplitude calculation).
    /// @param valid_entries How many oscillators should play (determined by the maximum available oscillators and the
    /// amplitudes above a given threshold).
    /// @return The summed output.
    T receive_output(const BinMagArr<T, (FFT_SIZE >> 1)>& bin_mag_arr, const size_t valid_entries) noexcept;

    /// @brief Tune every oscillator to it's appropriate frequency.
    /// @param bin_mag_arr Array containing pairs of the frequency bin and it's asociated amplitude (needed for
    /// frequency calculation).
    /// @param valid_entries How many oscillators should play (determined by the maximum available oscillators and the
    /// amplitudes above a given threshold).
    void tune_oscillators(const BinMagArr<T, (FFT_SIZE >> 1)>& bin_mag_arr, const size_t valid_entries) noexcept;

    /// @brief Reset all oscillators to a given sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    void reset(const double sampling_freq) noexcept;

    /// @brief Get another waveform for resynthesizing.
    /// @param osc_waveform The waveform the oscillators shall output (will only change their pointers to the respective
    /// wavetable).
    void select_waveform(const OscWaveform& osc_waveform) noexcept;

  public:
    // generate wavetables
    const SineWT<T, WT_SIZE> m_sin_wt{};
    const SquareWT<T, WT_SIZE> m_square_wt{};
    const TriWT<T, WT_SIZE> m_tri_wt{};
    const SawWT<T, WT_SIZE> m_saw_wt{};

  private:
    double m_sampling_freq;
    double m_freq_resolution;
    const T m_amp_correction = static_cast<T>(2) / FFT_SIZE;
    OscArray<T, WT_SIZE> m_osc_array;
};

/*
 * IMPLEMENTATION
 */
template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
T WTOscillator<T, WT_SIZE>::receive_output() noexcept
{
    // 1. determine the index as a whole number
    // 2. get the index above that number and wrap around if 1. is the last index of the WT
    const auto current_index = static_cast<size_t>(m_table_index);
    const size_t next_index = (current_index == WT_SIZE - 1) ? 0 : current_index + 1;
    // 3. get the values at these indices
    const T value_a = (*m_wt_ptr)[current_index];
    const T value_b = (*m_wt_ptr)[next_index];
    // 4. get the fraction offset and interpolate the output value
    const float fraction = m_table_index - current_index;
    const T output = value_a + fraction * (value_b - value_a);
    // 5. apply glide
     m_index_increment_old = m_index_increment_old + m_glide_fraction * (m_index_increment - m_index_increment_old);
     m_glide_fraction = ((m_glide_fraction += m_glide_increment) < 1.0f) ? m_glide_fraction : 1.0f;
    // 5. wrap if needed
    if ((m_table_index += m_index_increment_old) >= WT_SIZE)
    {
        m_table_index -= WT_SIZE;
    }
    return output;
}

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
void WTOscillator<T, WT_SIZE>::reset(const double sampling_freq) noexcept
{
    m_table_index = 0.0f;
    m_index_increment = 0.0f;
    m_sampling_freq = sampling_freq;
    m_nyquist_freq = sampling_freq / 2.0;
    m_inv_sampling_freq = 1.0 / sampling_freq;
}

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
void WTOscillator<T, WT_SIZE>::tune(T to_freq) noexcept
{
    m_index_increment_old = m_index_increment;
    // be sure not to tune above nyquist!
    if (to_freq > m_nyquist_freq)
    {
        to_freq = m_nyquist_freq;
    }
    // increment = N_WT * f0 / fs
    // steps from one sample to the next
    m_index_increment = WT_SIZE * to_freq * m_inv_sampling_freq;
}

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
ResynthOscs<T, WT_SIZE, FFT_SIZE>::ResynthOscs(const double sampling_freq)
    : m_sampling_freq{sampling_freq},
      m_freq_resolution{sampling_freq / static_cast<double>(FFT_SIZE)}
{
    for (auto& osc : m_osc_array)
    {
        osc = WTOscillator<T, WT_SIZE>{sampling_freq, &m_sin_wt};
    }
}

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
T ResynthOscs<T, WT_SIZE, FFT_SIZE>::receive_output(const BinMagArr<T, (FFT_SIZE >> 1)>& bin_mag_arr,
                                                    const size_t valid_entries) noexcept
{
    // valid entries is guaranteed to be smaller then max_oscillators!
    T summed_output = 0;
    for (size_t active_osc = 0; active_osc < valid_entries; ++active_osc)
    {
        summed_output += m_amp_correction * bin_mag_arr[active_osc].second * m_osc_array[active_osc].receive_output();
    }
    return summed_output;
}

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
void ResynthOscs<T, WT_SIZE, FFT_SIZE>::tune_oscillators(const BinMagArr<T, (FFT_SIZE >> 1)>& bin_mag_arr,
                                                         const size_t valid_entries) noexcept
{
    // valid entries is guaranteed to be smaller then max_oscillators!
    for (size_t active_osc = 0; active_osc < valid_entries; ++active_osc)
    {
        const T to_freq = bin_mag_arr[active_osc].first * m_freq_resolution;
        m_osc_array[active_osc].tune(to_freq);
    }
}

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
void ResynthOscs<T, WT_SIZE, FFT_SIZE>::reset(const double sampling_freq) noexcept
{
    m_sampling_freq = sampling_freq;
    m_freq_resolution = sampling_freq / FFT_SIZE;
    for (auto& osc : m_osc_array)
    {
        osc.reset(sampling_freq);
    }
}

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
void ResynthOscs<T, WT_SIZE, FFT_SIZE>::select_waveform(const OscWaveform& osc_waveform) noexcept
{

    const WaveTable<T, WT_SIZE>* wt_ptr = nullptr;
    switch (osc_waveform)
    {
    case OscWaveform::SINE:
        wt_ptr = &m_sin_wt;
        break;
    case OscWaveform::TRIANGLE:
        wt_ptr = &m_tri_wt;
        break;
    case OscWaveform::SAW:
        wt_ptr = &m_saw_wt;
        break;
    case OscWaveform::SQUARE:
        wt_ptr = &m_square_wt;
        break;
    }
    for (auto& osc : m_osc_array)
    {
        osc.change_waveform(wt_ptr);
    }
}

} // namespace LBTS::Spectral
