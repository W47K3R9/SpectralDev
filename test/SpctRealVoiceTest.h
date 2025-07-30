/**
 * Author: Lucas Scheidt
 * Date: 06.10.24
 *
 * Description:
 *
 */

#pragma once
#include "SpctBufferManager.h"
#include "VoiceSampleArray.h"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace LBTS::Spectral;
inline void test_real_voice()
{
    std::cout << "Testing real voice sample..." << std::endl;
    BufferManager<double, 16> test_bm{};

    constexpr auto one_twenty_four = BoundedPowTwo_v<size_t, 1024>;

    BufferManager<double> xl_buffer;
    std::ofstream txt_file_with_raw_sine{"real_voice.txt"};

    // VonHannWindow<double, BoundedPowTwo_v<size_t, 1024>> window{};
    std::ranges::for_each(VoiceExmp::sample_array,
                          [&](auto& el) { txt_file_with_raw_sine << std::setprecision(16) << el << std::endl; });

    const auto now = std::chrono::system_clock::now();
    double xl_array[one_twenty_four];
    std::ranges::copy(VoiceExmp::sample_array, xl_array);

    xl_buffer.set_voices(8);
    xl_buffer.process_daw_chunk(xl_array, one_twenty_four);

    const auto end = std::chrono::system_clock::now();
    std::cout << "Base case algorithm took " << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count()
              << " µs." << std::endl;

    // advance one iteration to get the first calculated output

    xl_buffer.process_daw_chunk(xl_array, one_twenty_four);
    xl_buffer.process_daw_chunk(xl_array, one_twenty_four);

    // xl_buffer.process_daw_chunk(xl_array, one_twenty_four, 1);
    //
    //
    std::ofstream txt_file_resynthesized{"resynthesized_voice.txt"};
    for (double i : xl_array)
    {
        txt_file_resynthesized << i << std::endl;
    }
}
