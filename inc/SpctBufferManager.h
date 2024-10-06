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
    explicit BufferManager(size_t t_size)
    {
        /// @todo custom container that chooses the right buffer size based on plugin preferences.
    }
    void process_daw_chunk(T* t_daw_chunk, size_t t_size);
    void reset_ring_buffers() noexcept { m_ring_buffers.reset_buffers(); }
  private:
    CTSampleBuffer<T, POTSamples<16>::value> m_ring_buffers{};
    const size_t m_buffer_size = m_ring_buffers.size();
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
     const unsigned steps_needed = t_size > m_buffer_size ?
         std::ceil(static_cast<float>(t_size)/m_buffer_size): 1; // NOLINT(*-narrowing-conversions)
    // clang-format on
    bool do_transformation = false;
    size_t daw_chunk_write_index = 0;
    for (unsigned step = 0; step < steps_needed; ++step)
    {
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
                i = 0.7333;
            }
            m_ring_buffers.fill_output(example);
            do_transformation = false;
        }
    }
}
} // namespace LBTS::Spectral
