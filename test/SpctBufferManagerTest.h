/**
 * Author: Lucas Scheidt
 * Date: 06.10.24
 *
 * Description:
 *
 */

#pragma once
#include "SpctInstanceController.h"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace LBTS::Spectral;
inline void test_buffer_manager()
{
    using SelecedType = double;
    InstanceController<SelecedType> fx_instance(44100.0);
    auto params = FxParameters{.waveform_selection = OscWaveform::SINE,
                               .filter_cutoff = 20000.0,
                               .fft_threshold = 0.01,
                               .frequency_offset = 0,
                               .gain = 2.0f,
                               .voices = 8,
                               .freeze = false};

    std::cout << "Testing circular buffers..." << std::endl;

    /// constexpr auto five_twelve = BoundedPowTwo_v<size_t, 512>;
    constexpr auto BUFFER_SIZE_REGULAR = BoundedPowTwo_v<size_t, 1024>;
    /// constexpr auto two_fourty_eight = BoundedPowTwo_v<size_t, 2048>;
    /// BufferManager<double, BoundedPowTwo_v<size_t, five_twelve>> l_buffer;
    /// double l_array[five_twelve];
    /// // l_buffer.process_daw_chunk(l_array, five_twelve);

    /// BufferManager<double> xl_buffer;
    std::ofstream txt_file_with_raw_sine{"raw_sine_values.txt"};
    std::array<SelecedType, BUFFER_SIZE_REGULAR> xl_array{};
    for (size_t ndx = 0; ndx < BUFFER_SIZE_REGULAR; ++ndx)
    {
        auto result = 0.4 * std::sin(6 * 2 * M_PI * static_cast<double>(ndx) / BUFFER_SIZE_REGULAR) +
                      0.8 * std::sin(10 * 2 * M_PI * static_cast<double>(ndx) / BUFFER_SIZE_REGULAR);
        xl_array.at(ndx) = static_cast<float>(result);
        txt_file_with_raw_sine << result << std::endl;
    }
    auto now = std::chrono::system_clock::now();
    /// xl_buffer.process_daw_chunk(xl_array, one_twenty_four);

    auto end = std::chrono::system_clock::now();
    std::cout << "Base case algorithm took " << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count()
              << " µs." << std::endl;

    // advance one iteration to get the first calculated output

    /// for (size_t i = 0; i < one_twenty_four; ++i)
    /// {
    ///     xl_array[i] = 0.4 * std::sin(6 * 2 * M_PI * static_cast<double>(i) / one_twenty_four) +
    ///                   0.8 * std::sin(10 * 2 * M_PI * static_cast<double>(i) / one_twenty_four);
    /// }
    /// xl_buffer.process_daw_chunk(xl_array, one_twenty_four);

    /// for (size_t i = 0; i < one_twenty_four; ++i)
    /// {
    ///     xl_array[i] = 0.4 * std::sin(6 * 2 * M_PI * static_cast<double>(i) / one_twenty_four) +
    ///                   0.8 * std::sin(10 * 2 * M_PI * static_cast<double>(i) / one_twenty_four);
    /// }
    /// xl_buffer.process_daw_chunk(xl_array, one_twenty_four);

    /// std::ofstream txt_file_resynthesized{"resynthesized_values.txt"};
    /// for (double i : xl_array)
    /// {
    ///     txt_file_resynthesized << i << std::endl;
    /// }

    /// double rising_value = 1.01;
    /// for (double& element : xl_array)
    /// {
    ///     element = rising_value;
    ///     rising_value += 0.02;
    /// }
    /// now = std::chrono::system_clock::now();
    /// xl_buffer.process_daw_chunk(xl_array, one_twenty_four);
    /// end = std::chrono::system_clock::now();
    /// std::cout << "Worst case algorithm took "
    ///           << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count() << " µs." << std::endl;

    // continous
    constexpr auto cycles = 5000u;
    now = std::chrono::system_clock::now();
    for (auto i = 0u; i < cycles; ++i)
    {
        fx_instance.update_parameters(params);
        fx_instance.process_daw_chunk(xl_array.data(), BUFFER_SIZE_REGULAR);
    }
    end = std::chrono::system_clock::now();
    std::cout << "Average of " << cycles << " cycles is "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count() / cycles << " µs."
              << std::endl;
    std::cout << "Total calculation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count()
              << " ms." << std::endl;
    std::cout << "Total time of processed audio with 44100 kHz f_s: " << cycles * BUFFER_SIZE_REGULAR / 44100.0
              << " seconds." << std::endl;

    std::cout << "Test passed." << std::endl;
}
