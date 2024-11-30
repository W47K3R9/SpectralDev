#pragma once
#include "SpctDomainSpecific.h"
#include <array>
#include <complex>

namespace LBTS::Spectral
{

template <typename T, size_t num_samples = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(num_samples))
void spct_fourier_transform(std::array<std::complex<T>, num_samples>& samples_arr) noexcept
{
    constexpr size_t degree_used = degree_value_of_pow<num_samples, size_t>();
    // first swap indices in bit-reversal manner
    // Algorithm:
    // for ( j = 0; j < n; ++j )
    //   q = j;
    //   r = 0
    //   for ( k = 0; k < log2(n); ++k )
    //      bk = q mod 2;
    //      q = q div 2;
    //      r = 2r + bk;
    //   if j < r then swap(xj, xr);
    for (size_t array_index = 0; array_index < num_samples; ++array_index)
    {
        size_t comparison = 0;
        for (size_t degree_index = 0; degree_index < degree_used; ++degree_index)
        {
            comparison = (comparison << 1) + ((array_index >> degree_index) & 1);
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
        //     all_twiddle_factors.choose_array(current_twiddle_factor_array_index);
        const size_t outer_limit = num_samples / current_pot;
        for (size_t outer_ndx = 0; outer_ndx < outer_limit; ++outer_ndx)
        {
            for (size_t inner_ndx = 0; inner_ndx < (current_pot >> 1); ++inner_ndx)
            {
                using namespace std::complex_literals;
                std::complex<double> tau =
                    std::exp(-2.0i * M_PI * static_cast<double>(inner_ndx) / static_cast<double>(current_pot)) *
                    // all_twiddle_factors[inner_ndx] *
                    samples_arr[outer_ndx * current_pot + inner_ndx + (current_pot >> 1)];
                samples_arr[outer_ndx * current_pot + inner_ndx + (current_pot >> 1)] =
                    samples_arr[outer_ndx * current_pot + inner_ndx] - tau;
                samples_arr[outer_ndx * current_pot + inner_ndx] += tau;
            }
        }
        ++current_twiddle_factor_array_index;
        current_pot <<= 1;
    }
}

} // namespace LBTS::Spectral

// OmegaArraySelector<double> all_twiddle_factors{};
//   constexpr size_t num_samples = 1024;
//   std::array<std::complex<double>, num_samples> samples = {0};
//   std::ofstream txt_file_with_raw_sine{"raw_sine_values.txt"};
//   for (size_t i = 0; i < num_samples; ++i)
//   {
//     samples[i] = std::sin(6 * 2 * M_PI * static_cast<double>(i) / num_samples);
//     txt_file_with_raw_sine << std::real(samples[i]) << std::endl;
//   }
//
//   const auto start = std::chrono::system_clock::now();
//   for (size_t array_index = 0; array_index < num_samples; ++array_index)
//   {
//     size_t var_q = array_index;
//     size_t var_r = 0;
//     for (size_t degree_index = 0; degree_index < 10; ++degree_index)
//     {
//       size_t var_b_k = var_q % 2;
//       var_q /= 2;
//       var_r = 2 * var_r + var_b_k;
//     }
//     if (array_index < var_r)
//     {
//       std::swap(samples[array_index], samples[var_r]);
//     }
//   }
//   // k = 2
//   // while ( k <= n )
//   //  for ( r = 0; r < n / k; ++r )
//   //    for ( i = 0; i < k / 2; ++i )
//   //      tau = W_k^i * x[rk+i+k/2]
//   //      x[rk+i+k/2] = x[rk+i] - tau
//   //      x[rk+i] = x[rk+i] + tau
//   //  k = 2k;
//   size_t coarse_ndx = 2; // k
//   size_t current_twiddle_factor_array_index = 0;
//   while (coarse_ndx <= num_samples)
//   {
//     all_twiddle_factors.choose_array(current_twiddle_factor_array_index);
//     size_t outer_limit = num_samples / coarse_ndx;
//     for (size_t outer_ndx = 0; outer_ndx < outer_limit; ++outer_ndx)
//     {
//       for (size_t inner_ndx = 0; inner_ndx < (coarse_ndx >> 1); ++inner_ndx)
//       {
//         using namespace std::complex_literals;
//         std::complex<double>
//             tau =  // std::exp(-2.0i * M_PI * static_cast<double>(inner_ndx) / static_cast<double>(coarse_ndx)) *
//              all_twiddle_factors[inner_ndx] *
//             samples[outer_ndx * coarse_ndx + inner_ndx + (coarse_ndx >> 1)];
//         samples[outer_ndx * coarse_ndx + inner_ndx + (coarse_ndx >> 1)] =
//             samples[outer_ndx * coarse_ndx + inner_ndx] - tau;
//         samples[outer_ndx * coarse_ndx + inner_ndx] += tau;
//       }
//     }
//     ++current_twiddle_factor_array_index;
//     coarse_ndx <<= 1;
//   }
//
//   const auto end = std::chrono::system_clock::now();
//   std::cout << "The algorithm took: " << (end - start).count() << " ns" << std::endl;
//   std::ofstream txt_file_with_transformed_values{"transformed_magnitudes.txt"};
//   for (auto& el : samples)
//   {
//     txt_file_with_transformed_values << std::abs(el) << std::endl;
//   }
//   txt_file_with_transformed_values.close();

// OmegaArray<double, 4> test_array{};
// for (auto& el : test_array)
//{
//   std::cout << std::abs(el) << ", " << std::arg(el) * 180 / M_PI << std::endl;
//   std::cout << std::real(el) << ", " << std::imag(el) << std::endl;
// }