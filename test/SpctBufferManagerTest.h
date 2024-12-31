/**
 * Author: Lucas Scheidt
 * Date: 06.10.24
 *
 * Description:
 *
 */

#pragma once
#include "SpctBufferManager.h"
#include <fstream>

static void dummy_fill(double* t_arr, const size_t t_arr_size)
{
    for (size_t index = 0; index < t_arr_size; ++index)
    {
        t_arr[index] = index + 0.5; // NOLINT(*-narrowing-conversions)
    }
}

using namespace LBTS::Spectral;
inline void test_buffer_manager()
{
    std::cout << "Testing circular buffers..." << std::endl;
    BufferManager<double, 16> test_bm{};

    constexpr auto five_twelve = BoundedPowTwo_v<size_t, 512>;
    constexpr auto one_twenty_four = BoundedPowTwo_v<size_t, 1024>;
    constexpr auto two_fourty_eight = BoundedPowTwo_v<size_t, 2048>;
    BufferManager<double, BoundedPowTwo_v<size_t, five_twelve>> l_buffer;
    double l_array[five_twelve];
    // l_buffer.process_daw_chunk(l_array, five_twelve);

    BufferManager<double> xl_buffer;
    std::ofstream txt_file_with_raw_sine{"raw_sine_values.txt"};
    double xl_array[one_twenty_four];
    for (size_t i = 0; i < one_twenty_four; ++i)
    {
        xl_array[i] = 0.3 * std::sin(6 * 2 * M_PI * static_cast<double>(i) / one_twenty_four) +
                      0.65 * std::sin(16 * 2 * M_PI * static_cast<double>(i) / one_twenty_four) +
                      0.9 * std::sin(10 * 2 * M_PI * static_cast<double>(i) / one_twenty_four);
        txt_file_with_raw_sine << xl_array[i] << std::endl;
    }
    auto now = std::chrono::system_clock::now();
    xl_buffer.process_daw_chunk(xl_array, one_twenty_four, 1);

    auto end = std::chrono::system_clock::now();
    std::cout << "Base case algorithm took " << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count()
              << " µs." << std::endl;

    // advance one iteration to get the first calculated output
    xl_buffer.process_daw_chunk(xl_array, one_twenty_four, 1);
    std::ofstream txt_file_resynthesized{"resynthesized_values.txt"};
    for (double i : xl_array)
    {
        txt_file_resynthesized << i << std::endl;
    }

    double rising_value = 1.01;
    for (double& element : xl_array)
    {
        element = rising_value;
        rising_value += 0.02;
    }
    now = std::chrono::system_clock::now();
    xl_buffer.process_daw_chunk(xl_array, one_twenty_four);
    end = std::chrono::system_clock::now();
    std::cout << "Worst case algorithm took "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count() << " µs." << std::endl;

    // continous
    constexpr auto cycles = 1000u;
    now = std::chrono::system_clock::now();
    for (auto i = 0u; i < cycles; ++i)
    {
        xl_buffer.process_daw_chunk(xl_array, one_twenty_four);
    }
    end = std::chrono::system_clock::now();
    std::cout << "Average of " << cycles << " cycles is "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count() / cycles << " µs."
              << std::endl;
    std::cout << "Total calculation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count()
              << " ms." << std::endl;
    std::cout << "Total time of processed audio with 44100 kHz f_s: " << cycles * one_twenty_four / 44100.0
              << " seconds." << std::endl;

    BufferManager<double, BoundedPowTwo_v<size_t, two_fourty_eight>> xxl_buffer;
    double xxl_array[two_fourty_eight];
    xxl_buffer.process_daw_chunk(xxl_array, two_fourty_eight);
    std::cout << "Test passed." << std::endl;
}