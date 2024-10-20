/**
 * Author: Lucas Scheidt
 * Date: 06.10.24
 *
 * Description:
 *
 */

#pragma once
#include "SpctBufferManager.h"

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
    BufferManager<double> test_bm(16);
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
    assert(larger_size[18] == 0.125);
    test_bm.reset_ring_buffers();

    double more_than_double[37];
    dummy_fill(more_than_double, 37);
    assert(more_than_double[18] == 18.5);
    test_bm.process_daw_chunk(more_than_double, 37);
    assert(more_than_double[15] == 0);
    assert(more_than_double[16] == 0.125);
    assert(more_than_double[36] == 0.125);
    test_bm.reset_ring_buffers();

    double smaller_size[8];
    dummy_fill(smaller_size, 8);
    assert(smaller_size[5] == 5.5);
    test_bm.process_daw_chunk(smaller_size, 8);
    assert(smaller_size[7] == 0);
    test_bm.process_daw_chunk(smaller_size, 8);
    assert(smaller_size[7] == 0);
    test_bm.process_daw_chunk(smaller_size, 8);
    assert(smaller_size[7] == 0.125);
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
    assert(smaller_odd_size[2] == 0.125);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(test_bm.ring_buffer_index() == 12);
    test_bm.process_daw_chunk(smaller_odd_size, 7);
    assert(test_bm.ring_buffer_index() == 3);
    test_bm.reset_ring_buffers();

    std::cout << "Test passed." << std::endl;
}