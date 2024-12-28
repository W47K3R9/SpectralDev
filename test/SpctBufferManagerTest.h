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
    double same_size[16];
    dummy_fill(same_size, 16);
    assert(same_size[10] == 10.5);
    test_bm.process_daw_chunk(same_size, 16);
    assert(same_size[6] == 0);
    test_bm.reset_ring_buffers();

    double larger_size[19];
    dummy_fill(larger_size, 19);
    test_bm.process_daw_chunk(larger_size, 19);
    assert(larger_size[15] == 0);
    // checked correctness successfully
    // assert(larger_size[18] == 0.125);
    test_bm.reset_ring_buffers();

    double more_than_double[37];
    dummy_fill(more_than_double, 37);
    assert(more_than_double[18] == 18.5);
    test_bm.process_daw_chunk(more_than_double, 37);
    assert(more_than_double[15] == 0);
    // assert(more_than_double[16] == 0.125);
    // assert(more_than_double[36] == 0.125);
    test_bm.reset_ring_buffers();

    double smaller_size[8];
    dummy_fill(smaller_size, 8);
    assert(smaller_size[5] == 5.5);
    test_bm.process_daw_chunk(smaller_size, 8);
    assert(smaller_size[7] == 0);
    test_bm.process_daw_chunk(smaller_size, 8);
    assert(smaller_size[7] == 0);
    test_bm.process_daw_chunk(smaller_size, 8);
    // assert(smaller_size[7] == 0.125);
    test_bm.reset_ring_buffers();

    double smaller_odd_size[7];
    dummy_fill(smaller_odd_size, 7);
    assert(smaller_odd_size[5] == 5.5);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(smaller_odd_size[5] == 0);
    assert(test_bm.ring_buffer_index() == 7);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(smaller_odd_size[5] == 0);
    assert(test_bm.ring_buffer_index() == 14);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(test_bm.ring_buffer_index() == 5);
    assert(smaller_odd_size[0] == 0);
    // assert(smaller_odd_size[2] == 0.125);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(test_bm.ring_buffer_index() == 12);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(test_bm.ring_buffer_index() == 3);
    test_bm.reset_ring_buffers();

    constexpr auto five_twelve = BoundedPowTwo_v<size_t, 512>;
    constexpr auto one_twenty_four = BoundedPowTwo_v<size_t, 1024>;
    constexpr auto two_fourty_eight = BoundedPowTwo_v<size_t, 2048>;
    BufferManager<double, BoundedPowTwo_v<size_t, five_twelve>> l_buffer;
    double l_array[five_twelve];
    l_buffer.process_daw_chunk(l_array, five_twelve);

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
              << std::chrono::duration_cast<std::chrono::microseconds>(end - now).count() / cycles << " µs." << std::endl;

    BufferManager<double, BoundedPowTwo_v<size_t, two_fourty_eight>> xxl_buffer;
    double xxl_array[two_fourty_eight];
    xxl_buffer.process_daw_chunk(xxl_array, two_fourty_eight);
    std::cout << "Test passed." << std::endl;
}