/**
 * Author: Lucas Scheidt
 * Date: 03.12.24
 *
 * Description: All processing functions that are needed for the plugin, this means an implementation of a non-recursive
 * FFT and also a determination of the k-highest amplitudes of the transformed signal.
 */

#pragma once
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include <algorithm>
#include <complex>

namespace LBTS::Spectral
{

/// @note DEG_TWO could be made uint8_t, but than pow_two_value_of_degree would have to be explicitely templated to
/// size_t (pow_two_value_of_degree<size_t>(uint8_t DEG_TWO) and since I'm lazy and on a computer we don't need to
/// be that greedy it's simply a size_t :)
template <FloatingPt T, size_t DEG_TWO = BoundedDegTwo<size_t, 10>::DEGREE>
    requires(is_bounded_degree(DEG_TWO))
void spct_fourier_transform(ComplexArr<T, pow_two_value_of_degree(DEG_TWO)>& samples_arr,
                            ExponentLUT<T>& exponent_lut) noexcept
{
    constexpr auto NUM_SAMPLES = pow_two_value_of_degree(DEG_TWO);
    // first swap indices in bit-reversal manner
    // Algorithm:
    // for ( j = 0; j < n; ++j )
    //   q = j;
    //   r = 0
    //   for ( k = 0; k < log2(n); ++k )
    //      bk = q mod 2; -> doable as & 1
    //      q = q div 2; -> doable via shift operator >>
    //      r = 2r + bk; -> doable via shift operator << and logical or
    //   if j < r then swap(xj, xr);
    for (size_t array_index = 0; array_index < NUM_SAMPLES; ++array_index)
    {
        size_t comparison = 0;
        for (size_t degree_index = 0; degree_index < DEG_TWO; ++degree_index)
        {
            comparison = (comparison << 1) | ((array_index >> degree_index) & 1);
        }
        if (array_index < comparison)
        {
            std::swap(samples_arr[array_index], samples_arr[comparison]);
        }
    }

    // calculate the actual fourier transform
    // Algorithm:
    // k = 2
    // while ( k <= n )
    //  for ( r = 0; r < n / k; ++r )
    //    for ( i = 0; i < k / 2; ++i )
    //      tau = W_k^i * x[rk+i+k/2]
    //      x[rk+i+k/2] = x[rk+i] - tau
    //      x[rk+i] = x[rk+i] + tau
    //  k = 2k;
    size_t current_pot = 2; // current power of two
    size_t current_exp_array_index = 0;
    while (current_pot <= NUM_SAMPLES)
    {
        exponent_lut.choose_array(current_exp_array_index);
        const size_t outer_limit = NUM_SAMPLES / current_pot;
        for (size_t outer_ndx = 0; outer_ndx < outer_limit; ++outer_ndx)
        {
            for (size_t inner_ndx = 0; inner_ndx < (current_pot >> 1); ++inner_ndx)
            {
                const auto butterfly_ndx_1 = outer_ndx * current_pot + inner_ndx;
                const auto butterfly_ndx_2 = butterfly_ndx_1 + (current_pot >> 1);
                const std::complex<T> tau = exponent_lut[inner_ndx] * samples_arr[butterfly_ndx_2];
                samples_arr[butterfly_ndx_2] = samples_arr[butterfly_ndx_1] - tau;
                samples_arr[butterfly_ndx_1] += tau;
            }
        }
        ++current_exp_array_index;
        current_pot <<= 1;
    }
}

template <FloatingPt T, size_t N_SAMPLES = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(N_SAMPLES))
void calculate_max_map(const ComplexArr<T, N_SAMPLES>& samples_arr, BinMagArr<T, (N_SAMPLES >> 1)>& bin_mag_arr,
                       const T threshold = MIN_GAIN_THRESHOLD<T>)
{
    // in order not to treat some arbitrary rounding errors like 1e-13 as valid magnitudes, everything beyond
    // MIN_GAIN_THRESHOLD is interpreted as 0.
    const auto clipped_threshold = std::clamp<T>(threshold, MIN_GAIN_THRESHOLD<T>, N_SAMPLES >> 1);
    size_t valid_entries = 0;
    for (size_t bin_number = 0; bin_number < N_SAMPLES >> 1; ++bin_number)
    {
        if (const auto abs_val = std::abs<T>(samples_arr[bin_number]); abs_val >= clipped_threshold)
        {
            bin_mag_arr[valid_entries] = std::make_pair(bin_number, abs_val);
            ++valid_entries;
        }
    }
    // sort the spectrum by descending magnitude (lambda)
    std::partial_sort(bin_mag_arr.begin(),
                      bin_mag_arr.begin() + valid_entries,
                      bin_mag_arr.end(),
                      [](const std::pair<size_t, T>& pair_a, const std::pair<size_t, T>& pair_b) -> bool
                      { return pair_a.second > pair_b.second; });
}

} // namespace LBTS::Spectral
