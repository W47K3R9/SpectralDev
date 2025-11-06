/**
 * Author: Lucas Scheidt
 * Date: 01.06.25
 *
 * Description: This is the building block of the sound engine. It contains a bunch of single oscillators that will
 * build the output sound.
 */
#pragma once
#include "SpctDomainSpecific.h"
#include "SpctOscillator.h"
#include "SpctWavetables.h"
#include <cmath>
#include <ranges>

namespace LBTS::Spectral
{
/// @brief Type definition for an array of wavetable oscillators.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
template <FloatingPt T, size_t WAVETABLE_SIZE, size_t NUM_OSCS = MAX_VOICES>
    requires(is_bounded_pow_two(WAVETABLE_SIZE) && NUM_OSCS <= MAX_VOICES)
using OscArray = std::array<WTOscillator<T, WAVETABLE_SIZE>, NUM_OSCS>;

/// @brief Object containing all oscillators that will resynthesize the FFT transformed input.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
/// @tparam FFT_SIZE Samples used for the fourier transformation.
template <FloatingPt T, size_t WAVETABLE_SIZE, size_t FFT_SIZE, size_t NUM_OSCS = MAX_VOICES>
    requires(is_bounded_pow_two(WAVETABLE_SIZE) && is_bounded_pow_two(FFT_SIZE))
class ResynthOscs
{
  public:
    ResynthOscs() = delete;
    /// @brief Correct instantiation with a valid sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    explicit ResynthOscs(const double sampling_freq)
        : m_sampling_freq{sampling_freq},
          m_freq_resolution{sampling_freq / static_cast<double>(FFT_SIZE)}
    {
        for (auto& osc : m_osc_array)
        {
            osc = WTOscillator<T, WAVETABLE_SIZE>{sampling_freq, &m_sin_wt};
        }
    }
    ResynthOscs(const ResynthOscs&) = delete;
    ResynthOscs& operator=(const ResynthOscs&) = delete;
    ResynthOscs(ResynthOscs&&) noexcept = delete;
    ResynthOscs& operator=(ResynthOscs&&) = delete;
    ~ResynthOscs() = default;

    /// @brief Get the summed output of all playing oscillators.
    /// @return The summed output.
    T receive_output() noexcept
    {
        // valid entries is guaranteed to be smaller then max_oscillators!
        T summed_output = 0;
        std::ranges::for_each(m_osc_array,
                              [&summed_output](auto& osc) { summed_output += osc.advance_and_receive_output(); });
        return summed_output;
    }

    /// @brief Tune every oscillator to it's appropriate frequency.
    /// @param bin_mag_arr Array containing pairs of the frequency bin and it's asociated amplitude (needed for
    /// frequency calculation).
    /// @param num_voices How many oscillators shall be tuned, note that if this number is higher than the oscillator
    /// count managed by an ResynthOscs block, the higher numbers will be ignored.
    void tune_oscillators_to_fft(const BinMagArr<T, (FFT_SIZE / 2)>& bin_mag_arr, const size_t num_voices) noexcept
    {
        // e.g. num_voices = 4, NUM_OSCS = 8:
        // -> active_oscs = quiet_oscs = 4
        // -> range active: from bin_mag_arr.begin() to active_oscs
        const auto num_active_oscs = std::clamp<size_t>(num_voices, 0, NUM_OSCS);
        const auto num_silent_oscs = NUM_OSCS - num_active_oscs;

        // valid valid_entries is guaranteed to be smaller then max_oscillators!
        size_t nth_osc = 0;
        // const auto active_entries = std::views::counted(bin_mag_arr.begin(), num_active_oscs);
        const auto active_entries = std::views::counted(bin_mag_arr.begin(), num_voices);
        std::ranges::for_each(active_entries,
                              [&nth_osc, this](const auto& entry)
                              {
                                  m_osc_array[nth_osc].tune_and_set_amp(entry.first * m_freq_resolution + m_freq_offset,
                                                                        entry.second * m_amp_correction);
                                  nth_osc += 1;
                              });
        const auto silent_entries = std::views::counted(bin_mag_arr.begin() + num_active_oscs, num_silent_oscs);
        std::ranges::for_each(silent_entries,
                              [&nth_osc, this]([[maybe_unused]] const auto& entry)
                              {
                                  m_osc_array[nth_osc].tune_and_set_amp(0, 0);
                                  nth_osc += 1;
                              });
    }

    /// @brief Muting means set frequency and amplitude of all oscillators to zero.
    void mute_oscillators() const noexcept
    {
        for (auto& osc : m_osc_array)
        {
            osc.tune_and_set_amp(0, 0);
        }
    }

    /// @brief Will be added to the calculated FFT frequency
    /// @param freq_offset: in Hz
    void set_frequency_offset(float freq_offset) noexcept { m_freq_offset = static_cast<T>(freq_offset); }

    /// @brief Reset all oscillators to a given sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    void reset(const double sampling_freq) noexcept
    {
        m_freq_offset = 0;
        m_sampling_freq = sampling_freq;
        m_freq_resolution = sampling_freq / FFT_SIZE;
        for (auto& osc : m_osc_array)
        {
            osc.reset(sampling_freq);
        }
    }

    /// @brief Get another waveform for resynthesizing.
    /// @param osc_waveform The waveform the oscillators shall output (will only change their pointers to the respective
    /// wavetable).
    void select_waveform(const OscWaveform& osc_waveform) noexcept
    {

        const WaveTable<T, WAVETABLE_SIZE>* wt_ptr = nullptr;
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
            osc.select_waveform(wt_ptr);
        }
    }

    /// @brief Avoids clicks and pops when tuning from one frequency set to the next.
    /// @param glide_steps: Samples it will take to transition from one amplitude / frequency to the next.
    void set_glide_steps(uint16_t glide_steps) noexcept
    {
        for (auto& osc : m_osc_array)
        {
            osc.set_glide_steps(glide_steps);
        }
    }

  private:
    // generate wavetables
    const SineWT<T, WAVETABLE_SIZE> m_sin_wt{};
    const SquareWT<T, WAVETABLE_SIZE> m_square_wt{};
    const TriWT<T, WAVETABLE_SIZE> m_tri_wt{};
    const SawWT<T, WAVETABLE_SIZE> m_saw_wt{};
    double m_sampling_freq;
    double m_freq_resolution;
    const T m_amp_correction = static_cast<T>(2) / FFT_SIZE;
    OscArray<T, WAVETABLE_SIZE, NUM_OSCS> m_osc_array;
    std::atomic<T> m_freq_offset = 0;
};
} // namespace LBTS::Spectral
