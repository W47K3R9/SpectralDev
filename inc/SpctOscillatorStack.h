/**
 * Author: Lucas Scheidt
 * Date: 01.06.25
 *
 * Description: This is the building block of the sound engine.
 */

#pragma once
#include "SpctOscillator.h"
#include "SpctWavetables.h"

namespace LBTS::Spectral
{
/// @brief Type definition for an array of wavetable oscillators.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
template <FloatingPt T, size_t WT_SIZE, size_t NUM_OSCS = max_oscillators>
    requires(is_bounded_pow_two(WT_SIZE) && NUM_OSCS <= max_oscillators)
using OscArray = std::array<WTOscillator<T, WT_SIZE>, NUM_OSCS>;

/// @brief Object containing all oscillators that will resynthesize the FFT transformed input.
/// @tparam T The type of the wavetable entries.
/// @tparam WT_SIZE The size of the wavetable that will be read.
/// @tparam FFT_SIZE Samples used for the fourier transformation.
template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE, size_t NUM_OSCS = max_oscillators,
          size_t HALF_FFT_SIZE = FFT_SIZE / 2u>
    requires(is_bounded_pow_two(WT_SIZE))
class ResynthOscs
{
  public:
    /// @brief Avoid instantiation with a wrong sampling frequency.
    ResynthOscs() = delete;

    /// @brief Correct instantiation with a valid sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    explicit ResynthOscs(const double sampling_freq)
        : m_sampling_freq{sampling_freq},
          m_freq_resolution{sampling_freq / static_cast<double>(FFT_SIZE)}
    {
        for (auto& osc : m_osc_array)
        {
            osc = WTOscillator<T, WT_SIZE>{sampling_freq, &m_sin_wt};
        }
    }

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
    /// @return The summed output.
    T receive_output() noexcept
    {
        // valid entries is guaranteed to be smaller then max_oscillators!
        T summed_output = 0;
        // I don't know if ranges has some advantages...
        // std::ranges::for_each(m_osc_array,
        // [&summed_output, this](auto& osc) { summed_output += osc.advance_and_receive_output(); });
        for (size_t active_osc = 0; active_osc < NUM_OSCS; ++active_osc)
        {
            summed_output += m_osc_array[active_osc].advance_and_receive_output();
        }
        return m_amp_correction * summed_output;
    }

    /// @brief Tune every oscillator to it's appropriate frequency.
    /// @param bin_mag_arr Array containing pairs of the frequency bin and it's asociated amplitude (needed for
    /// frequency calculation).
    /// @param valid_entries How many oscillators should play (determined by the maximum available oscillators and the
    /// amplitudes above a given threshold).
    void tune_oscillators(const BinMagArr<T, HALF_FFT_SIZE>& bin_mag_arr, const size_t valid_entries) noexcept
    {
        // valid entries is guaranteed to be smaller then max_oscillators!
        for (size_t active_osc = 0; active_osc < valid_entries; ++active_osc)
        {
            const T to_freq = bin_mag_arr[active_osc].first * m_freq_resolution;
            m_osc_array[active_osc].tune(to_freq);
        }
    }

    /// @brief Tune every oscillator to it's appropriate frequency.
    /// @param bin_mag_arr Array containing pairs of the frequency bin and it's asociated amplitude (needed for
    /// frequency calculation).
    /// @param entries How many oscillators shall be tuned, note that if this number is higher than the oscillator count
    /// managed by an ResynthOscs block, the higher numbers will be ignored.
    /// @param from_index Starting of the FFT index, be sure that from_index + entries > bin_mag_arr.size, not to access
    /// an invalid array position.
    void tune_oscillators_to_fft(const BinMagArr<T, HALF_FFT_SIZE>& bin_mag_arr, const size_t entries,
                                 const size_t from_index = 0) noexcept
    {
        // std::cout << "entries: " << entries << '\n';
        assert(from_index + entries < bin_mag_arr.size());
        // you can't tune more oscillators than you have.
        const auto accessible_entries = std::clamp<size_t>(entries, 0, NUM_OSCS);
        const auto relevant_entries = std::views::counted(bin_mag_arr.begin() + from_index, accessible_entries);
        // valid entries is guaranteed to be smaller then max_oscillators!
        size_t nth_osc = 0;
        std::ranges::for_each(relevant_entries,
                              [&nth_osc, this](const auto& entry)
                              {
                                  set_amp_oscillator_n(nth_osc, entry.second);
                                  tune_oscillator_n(nth_osc, entry.first * m_freq_resolution);
                                  nth_osc += 1;
                              });
    }

    /// @brief Reset all oscillators to a given sampling frequency.
    /// @param sampling_freq Determined by the DAW.
    void reset(const double sampling_freq) noexcept
    {
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

  private:
    [[nodiscard]] size_t clip_nth_osc(const size_t nth_osc) const noexcept
    {
        return std::clamp<size_t>(nth_osc, 0, NUM_OSCS);
    }

    void tune_oscillator_n(const size_t nth_osc, const T freq) { m_osc_array[clip_nth_osc(nth_osc)].tune(freq); }
    void set_amp_oscillator_n(const size_t nth_osc, const T amp)
    {
        m_osc_array[clip_nth_osc(nth_osc)].set_amplitude(amp);
    }

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
    OscArray<T, WT_SIZE, NUM_OSCS> m_osc_array;
};
} // namespace LBTS::Spectral