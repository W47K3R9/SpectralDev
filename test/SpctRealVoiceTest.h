/**
 * Author: Lucas Scheidt
 * Date: 06.10.24
 *
 * Description:
 *
 */

#pragma once
#include "SpctBufferManager.h"
#include "SpctDomainSpecific.h"
#include "SpctFxParameters.h"
#include "SpctInstanceController.h"
#include "VoiceSampleArray.h"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace LBTS::Spectral;
inline void test_real_voice()
{
    constexpr auto one_twenty_four = BoundedPowTwo_v<size_t, 1024>;
    InstanceController fx_instance(44100.0);
    std::ofstream txt_file_with_raw_sine{"real_voice.txt"};

    // VonHannWindow<double, BoundedPowTwo_v<size_t, 1024>> window{};
    std::ranges::for_each(VoiceExmp::sample_array,
                          [&](auto& element)
                          { txt_file_with_raw_sine << std::setprecision(16) << element << std::endl; });

    float xl_array[one_twenty_four];
    std::ranges::copy(VoiceExmp::sample_array, xl_array);
    auto params = FxParameters{.waveform_selection = OscWaveform::SINE,
                               .filter_cutoff = 20000.0,
                               .fft_threshold = 0.01,
                               .frequency_offset = 0,
                               .gain = 1.0,
                               .voices = 4,
                               .freeze = false};
    const auto now = std::chrono::system_clock::now();
    fx_instance.update_parameters(params);
    fx_instance.process_daw_chunk(xl_array, one_twenty_four);
    const auto end = std::chrono::system_clock::now();
    std::cout << "Base case algorithm took " << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count()
              << " µs." << std::endl;

    // advance one iteration to get the first calculated output

    fx_instance.process_daw_chunk(xl_array, one_twenty_four);
    fx_instance.process_daw_chunk(xl_array, one_twenty_four);

    std::ofstream txt_file_resynthesized{"resynthesized_voice.txt"};
    for (double i : xl_array)
    {
        txt_file_resynthesized << i << std::endl;
    }
}
