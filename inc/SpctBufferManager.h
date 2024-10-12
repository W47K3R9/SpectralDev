/**
 * The buffer size manager is the first stage of the audio plugin.
 * It collects the amount of samples specified by the ControlPanel and forwards
 * them to the FourierMap.
 */
#pragma once
#include "SpctDomainSpecific.h"
#include <cmath>

/**
 * DECLARATION
 */
namespace LBTS::Spectral
{
template <typename T>
class BufferManager
{
  public:
    BufferManager() = default;
    explicit BufferManager(size_t i_range)
    {
        m_ring_buffers.resize_valid_range(i_range);
        m_buffer_size = m_ring_buffers.size();
    }
    void process_daw_chunk(T* t_daw_chunk, size_t t_size);
    void reset_ring_buffers() noexcept { m_ring_buffers.reset_buffers(); }

    /// @note this is only needed for testing purposes, could be deletet later on.
    [[nodiscard]] size_t ring_buffer_index() const noexcept { return m_ring_buffers.current_index(); }
  private:
    CircularSampleBuffer<T> m_ring_buffers{};
    size_t m_buffer_size = m_ring_buffers.size();
};

/**
 * IMPLEMENTATION
 */
template <typename T>
void BufferManager<T>::process_daw_chunk(T* t_daw_chunk, const size_t t_size)
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
            m_ring_buffers.fill_input(t_daw_chunk[daw_chunk_write_index]);
            t_daw_chunk[daw_chunk_write_index] = m_ring_buffers.receive_output();
            do_transformation = m_ring_buffers.advance();
            ++daw_chunk_write_index;
        }
        if (do_transformation)
        {
            /// @note pass the whole array to FFT.
            T example[16]{};
            for (auto& i : example)
            {
                i = 0.125;
            }
            m_ring_buffers.fill_output_unsafe(example);
            do_transformation = false;
        }
    }
    // tackle rest if chunk < internal size and while has been interrupted by transform flag
    if (steps_needed == 1)
    {
        while (daw_chunk_write_index < t_size)
        {
            m_ring_buffers.fill_input(t_daw_chunk[daw_chunk_write_index]);
            t_daw_chunk[daw_chunk_write_index] = m_ring_buffers.receive_output();
            m_ring_buffers.advance();
            ++daw_chunk_write_index;
        }
    }
}
} // namespace LBTS::Spectral
