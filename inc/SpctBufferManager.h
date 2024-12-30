/**
 * The buffer size manager is the first stage of the audio plugin.
 * It collects the amount of samples specified by the ControlPanel and forwards
 * them to the FourierMap.
 */
#pragma once
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include "SpctOscillators.h"
#include "SpctProcessingFunctions.h"
#include <cmath>

/**
 * DECLARATION
 */
namespace LBTS::Spectral
{
template <FloatingPt T, size_t BUFFER_SIZE = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(BUFFER_SIZE))
class BufferManager
{
  public:
    BufferManager() = default;
    explicit BufferManager(const double sampling_freq) : m_sampling_freq{sampling_freq}, m_oscillators{sampling_freq} {}
    /// @note the runtime implementation of a changing size for the fft may be more complex than expected and will
    /// therefore be postponed.
    // explicit BufferManager(const size_t i_range)
    // {
    //     m_ring_buffers.resize_valid_range(i_range);
    //     m_buffer_size = m_ring_buffers.size();
    // }

    /// @brief main processing is done in here since Juce works with C-style arrays this takes in T* as first address
    /// of the array.
    void process_daw_chunk(T* daw_chunk, const size_t t_size, const T threshold = 1.0);

    void reset(const double sampling_freq) noexcept
    {
        m_sampling_freq = sampling_freq;
        reset_ring_buffers();
    }

    void reset_ring_buffers() noexcept { m_ring_buffers.reset_buffers(); }

    /// @note this is only needed for testing purposes, could be deletet later on.
    [[nodiscard]] size_t ring_buffer_index() const noexcept { return m_ring_buffers.current_index(); }

  private:
    CircularSampleBuffer<T, BUFFER_SIZE> m_ring_buffers{};
    size_t m_buffer_size = m_ring_buffers.size();
    ExponentLUT<T> m_exponent_lut{};
    BinMagArr<T, (BUFFER_SIZE >> 1)> m_bin_mag_arr;
    // Juce uses double as sample frequency, since I'll use the framework for deployment I'll use double too.
    double m_sampling_freq = 44100;
    ResynthOscs<T, BoundedPowTwo_v<size_t, 512>, BUFFER_SIZE> m_oscillators{m_sampling_freq};
};

/**
 * IMPLEMENTATION
 */
template <FloatingPt T, size_t BUFFER_SIZE>
    requires(is_bounded_pow_two(BUFFER_SIZE))
void BufferManager<T, BUFFER_SIZE>::process_daw_chunk(T* daw_chunk, const size_t t_size, const T threshold)
{
    // in case the daw buffer is greater that the internal one, more steps are needed.
    // The narrowing conversion is WANTED!
    // clang-format off
     const size_t steps_needed = t_size > m_buffer_size ?
         std::ceil(static_cast<float>(t_size)/m_buffer_size) : 1; // NOLINT(*-narrowing-conversions)
    // clang-format on
    size_t daw_chunk_write_index = 0;
    bool do_transformation = false;
    for (size_t step = 0; step < steps_needed; ++step)
    {
        // in case that chunk > internal size this while loop takes care of the complete filling porcess correctly
        while (!do_transformation && daw_chunk_write_index < t_size)
        {
            m_ring_buffers.fill_input(daw_chunk[daw_chunk_write_index]);
            daw_chunk[daw_chunk_write_index] = m_ring_buffers.receive_output();
            do_transformation = m_ring_buffers.advance();
            ++daw_chunk_write_index;
        }
        if (do_transformation)
        {
            // pass the whole array as reference to the FFT, will change the input array!
            spct_fourier_transform<T, degree_of_pow_two_value(BUFFER_SIZE)>(m_ring_buffers.m_in_array, m_exponent_lut);
            // calculate the dominant magnitudes, won't change the input array
            const auto valid_entries =
                calculate_max_map<T, BUFFER_SIZE>(m_ring_buffers.m_in_array, m_bin_mag_arr, threshold);
            // fill the output array
            resynthesize_output<T, BUFFER_SIZE>(m_ring_buffers.m_out_array, m_bin_mag_arr, valid_entries);
            do_transformation = false;
        }
    }
    // tackle rest if chunk < internal size and while has been interrupted by transform flag
    if (steps_needed == 1)
    {
        while (daw_chunk_write_index < t_size)
        {
            m_ring_buffers.fill_input(daw_chunk[daw_chunk_write_index]);
            daw_chunk[daw_chunk_write_index] = m_ring_buffers.receive_output();
            m_ring_buffers.advance();
            ++daw_chunk_write_index;
        }
    }
}
} // namespace LBTS::Spectral
