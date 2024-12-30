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

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
class WTOscillator
{
  public:
    WTOscillator() = default;
    ~WTOscillator() = default;
    T output_value() { return 0; }
    void tune(T to_freq) { to_freq = 0; }

  private:
    WaveTable<T, WT_SIZE>* m_wt_ptr;
};

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
using OscArray = std::array<WTOscillator<T, WT_SIZE>, max_oscillators>;

template <FloatingPt T, size_t WT_SIZE, size_t FFT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
class ResynthOscs
{
  public:
    // avoid wrong instantiation of sampling frequency!
    ResynthOscs() = delete;
    explicit ResynthOscs(const double sampling_freq)
        : m_sampling_freq{sampling_freq}, m_freq_resolution{m_sampling_freq / FFT_SIZE}
    {
        for (auto& osc : m_osc_array)
        {
            osc.tune(0);
        }
    }
    ~ResynthOscs() = default;
    T output_value(const BinMagArr<T, FFT_SIZE>& bin_mag_arr, const size_t valid_entries) noexcept
    {
        const auto oscs_to_play = limit_active_oscs(valid_entries);
        T summed_output = 0;
        for (size_t active_osc = 0; active_osc < oscs_to_play; ++active_osc)
        {
            summed_output += m_amp_correction * bin_mag_arr[active_osc].second * m_osc_array[active_osc].output_value();
        }
        return summed_output;
    }
    void tune_oscillators(const BinMagArr<T, FFT_SIZE>& bin_mag_arr, const size_t valid_entries) noexcept
    {
        const auto oscs_to_tune = limit_active_oscs(valid_entries);
        for (size_t active_osc = 0; active_osc < oscs_to_tune; ++active_osc)
        {
            const T to_freq = bin_mag_arr[active_osc].first * m_freq_resolution;
            m_osc_array[active_osc].tune(to_freq);
        }
    }

  private:
    [[nodiscard]] size_t limit_active_oscs(const size_t valid_entries) const noexcept
    {
        return valid_entries <= max_oscillators ? valid_entries : max_oscillators;
    }

  private:
    OscArray<T, WT_SIZE> m_osc_array{};
    double m_sampling_freq;
    double m_freq_resolution;
    const double m_amp_correction = static_cast<T>(2) / FFT_SIZE;
    SineWT<T, WT_SIZE> m_sin_wt {};
    SquareWT<T, WT_SIZE> m_square_wt {};
    TriWT<T, WT_SIZE> m_tri_wt {};
};

} // namespace LBTS::Spectral