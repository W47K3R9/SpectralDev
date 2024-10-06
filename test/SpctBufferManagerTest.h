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
        t_arr[index] = 0.1 * index; // NOLINT(*-narrowing-conversions)
    }
}

using namespace LBTS::Spectral;
inline void test_buffer_manager()
{
    BufferManager<double> test_bm;
    double same_size[16];
    dummy_fill(same_size, 16);
    test_bm.process_daw_chunk(same_size, 16);
    test_bm.reset_ring_buffers();

    double larger_size[19];
    dummy_fill(larger_size, 19);
    test_bm.process_daw_chunk(larger_size, 19);
    test_bm.reset_ring_buffers();

    double more_than_double[37];
    dummy_fill(more_than_double, 37);
    test_bm.process_daw_chunk(more_than_double,37);
    test_bm.reset_ring_buffers();

    double smaller_size[8];
    dummy_fill(smaller_size, 8);
    test_bm.process_daw_chunk(smaller_size, 8);
    test_bm.process_daw_chunk(smaller_size, 8);
    test_bm.process_daw_chunk(smaller_size, 8);
    test_bm.reset_ring_buffers();

    double smaller_odd_size[7];
    dummy_fill(smaller_odd_size, 7);
    test_bm.process_daw_chunk(smaller_odd_size,7);
    test_bm.process_daw_chunk(smaller_odd_size,7);
    test_bm.process_daw_chunk(smaller_odd_size,7);
    test_bm.process_daw_chunk(smaller_odd_size,7);
    test_bm.reset_ring_buffers();
}