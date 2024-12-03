#pragma once
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include <array>
#include <complex>

// DEBUG
#include <iostream>

namespace LBTS::Spectral
{

/// @note DEG_TWO could be made uint8_t, but than pow_two_value_of_degree would have to be explicitely templated to
/// size_t (pow_two_value_of_degree<size_t>(uint8_t DEG_TWO) and since I'm lazy and on a computer we don't need to
/// be that greedy it's simply a size_t :)
template <typename T, size_t DEG_TWO = BoundedDegTwo<size_t, 10>::degree>
    requires(is_bounded_degree(DEG_TWO))
void spct_fourier_transform(std::array<std::complex<T>, pow_two_value_of_degree(DEG_TWO)>& samples_arr,
                            ExponentLUT<T>& exponent_lut) noexcept
{
    // DEBUG: BEGIN
    const auto start = std::chrono::system_clock::now();

    constexpr auto num_samples = pow_two_value_of_degree(DEG_TWO);
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
    for (size_t array_index = 0; array_index < num_samples; ++array_index)
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
    size_t current_twiddle_factor_array_index = 0;
    while (current_pot <= num_samples)
    {
        exponent_lut.choose_array(current_twiddle_factor_array_index);
        const size_t outer_limit = num_samples / current_pot;
        for (size_t outer_ndx = 0; outer_ndx < outer_limit; ++outer_ndx)
        {
            for (size_t inner_ndx = 0; inner_ndx < (current_pot >> 1); ++inner_ndx)
            {
                using namespace std::complex_literals;
                std::complex<double> tau =
                   exponent_lut[inner_ndx] * samples_arr[outer_ndx * current_pot + inner_ndx + (current_pot >> 1)];
                samples_arr[outer_ndx * current_pot + inner_ndx + (current_pot >> 1)] =
                    samples_arr[outer_ndx * current_pot + inner_ndx] - tau;
                samples_arr[outer_ndx * current_pot + inner_ndx] += tau;
            }
        }
        ++current_twiddle_factor_array_index;
        current_pot <<= 1;
    }
    const auto end = std::chrono::system_clock::now();
    std::cout << "The algorithm took: " << (end - start).count() << " ns" << std::endl;
}

} // namespace LBTS::Spectral