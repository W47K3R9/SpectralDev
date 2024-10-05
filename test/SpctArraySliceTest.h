/**
 * Author: Lucas Scheidt
 * Date: 28.09.24
 *
 * Description: Test cases for the array view inside SpctDomainSpecific.h
 */

#pragma once
#include "SpctDomainSpecific.h"

#include <array>
#include <cassert>
#include <iostream>
#include <ranges>

namespace LBTS::Spectral
{

template <typename ArrT, typename SliT = ArrT, size_t large_size = 64>
void print_comparison(ArrT t_array, SliT t_slice)
{
    std::cout << "Full Array: [ ";
    for(size_t i = 0; i < large_size; ++i)
        std::cout << t_array[i] << " ";
    std::cout << "]\nSlice of the full array: [ ";
    std::ranges::copy(t_slice, std::ostream_iterator<typename SliT::type>(std::cout, " "));
    std::cout << "]" << std::endl;
}

inline void test_array_slice()
{
    std::cout << "Testing array slicing..." << std::endl;
    constexpr unsigned large_size = POTSamples<64>::value;
    constexpr unsigned slice_size = POTSamples<16>::value;
    double samples[large_size] = {};
    for (unsigned i = 0; i < slice_size; ++i)
    {
        samples[i] = i * 1.0;
    }
    StaticSampleArrayView<double, slice_size, large_size> sample_view(samples);

    // Check validity of reference
    samples[0] = 17.0;
    assert(sample_view[0] == 17);
    assert(sample_view[1] == 1.0);

    // Check the advance function
    for (unsigned i = slice_size; i < 2 * slice_size; ++i)
    {
        samples[i] = i / 10.0;
    }
    sample_view.advance();
    assert(*sample_view.begin() == 1.6);
    // print_comparison(samples, sample_view);

    // Check copy of C style array
    double c_array_to_copy[slice_size] = {0.11, 0.22, 0.33, 0.44};
    sample_view.write_to_slice_unsafe(c_array_to_copy);
    assert(samples[16] == 0.11);
    // print_comparison(samples, sample_view);

    // Check copy of std::arry
    std::array<double, 16> std_array_to_copy{1, 3, 5, 7};
    sample_view.write_to_slice(std_array_to_copy);
    assert(samples[16] == 1);
    // print_comparison(samples, sample_view);

    // Check cycle through boundaries
    sample_view.advance();
    sample_view.advance();
    sample_view.advance();
    assert(sample_view[0] == 17);
    assert(sample_view[16] == 17);
    assert(sample_view[18] == 2.0);
    std::cout << "Test passed." << std::endl;
}
} // namespace LBTS::Spectral