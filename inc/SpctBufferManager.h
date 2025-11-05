/**
 * Author: Lucas Scheidt
 * Date: 13.10.25
 *
 * Description: The buffer manager is responsible to firstly get the input samples into the circular buffer in order for
 * the calculation manager to transform its contents and secondly to replace the samples of the DAW input with the
 * output of the resynthesizing oscillators.
 */
#pragma once
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctOscillatorStack.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <memory>

namespace LBTS::Spectral
{
template <FloatingPt T, size_t FFT_SIZE = BoundedPowTwo_v<size_t, 1024>,
          size_t WAVETABLE_SIZE = BoundedPowTwo_v<size_t, 256>>
    requires(is_bounded_pow_two(FFT_SIZE) && is_bounded_pow_two(WAVETABLE_SIZE))
class BufferManager
{
  public:
    /// @brief Constructor with all necessary initializations.
    /// @param sampling_freq: Regularly set by the DAW, defaults to 44.100 Hz
    /// @param resynth_osc_ptr: Oscillators (managed from and tuned from external)
    /// @param calculation_sync_primitives: Synchronization for the FFT calculation. The BufferManager notifies the
    /// condition variable as soon as a buffer is filled and ready to be transformed.
    BufferManager(const double sampling_freq,
                  std::shared_ptr<CircularSampleBuffer<T, FFT_SIZE, WAVETABLE_SIZE>> circular_buffer_ptr,
                  std::shared_ptr<ResynthOscs<T, WAVETABLE_SIZE, FFT_SIZE>> resynth_osc_ptr,
                  std::shared_ptr<SyncPrimitives> calculation_sync_primitives)
        : m_sampling_freq{sampling_freq},
          m_circular_buffer_ptr{std::move(circular_buffer_ptr)},
          m_resynth_oscs_ptr{std::move(resynth_osc_ptr)},
          m_calculation_sp_ptr{std::move(calculation_sync_primitives)}
    {}
    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;
    BufferManager(BufferManager&&) = delete;
    BufferManager& operator=(BufferManager&&) = delete;
    ~BufferManager() = default;

    void set_cutoff(const float freq)
    {
        m_alpha = 1.0 - std::exp(-TWO_PI<double> * static_cast<double>(freq) / m_sampling_freq);
    }

    void set_gain(const float gain) { m_gain = std::clamp<float>(gain, 0.0f, 2.0f); }

    /// @brief main processing is done in here since Juce works with C-style arrays this takes in T* as first address
    /// of the array.
    void process_daw_chunk(T* daw_chunk, const size_t t_size)
    {
        assert(m_circular_buffer_ptr != nullptr);
        assert(m_calculation_sp_ptr != nullptr);
        assert(m_resynth_oscs_ptr != nullptr);
        // in case the daw buffer is greater then the internal one, more steps are needed.
        // The narrowing conversion is WANTED!
        const size_t steps_needed =
            t_size > FFT_SIZE ? std::ceil(static_cast<float>(t_size) / FFT_SIZE) : 1; // NOLINT(*-narrowing-conversions)
        size_t daw_chunk_write_index = 0;
        const auto fill_in_and_out_buffers = [this, &daw_chunk_write_index, &daw_chunk]()
        {
            // NOLINTBEGIN(*-pointer-arithmetic)
            m_circular_buffer_ptr->fill_input(daw_chunk[daw_chunk_write_index]);
            m_previous_sample =
                (1.0 - m_alpha) * m_previous_sample + m_alpha * m_resynth_oscs_ptr->receive_output() * m_gain;
            /// @todo refine feedback behaviour!
            /// @note ... or let it be because feedback in digital processes is essentally just delay, which would be
            /// dumb...
            daw_chunk[daw_chunk_write_index] = m_previous_sample;
            // NOLINTEND(*-pointer-arithmetic)
            ++daw_chunk_write_index;
        };

        for (size_t step = 0; step < steps_needed; ++step)
        {
            // in case that chunk > internal size this while loop takes care of the complete filling porcess correctly
            while (!m_initiate_fft && daw_chunk_write_index < t_size)
            {
                fill_in_and_out_buffers();
                m_initiate_fft = m_circular_buffer_ptr->advance();
            }
            if (m_initiate_fft && m_calculation_sp_ptr->action_done)
            {
                m_circular_buffer_ptr->copy_to_output();
                m_calculation_sp_ptr->signalling_cv.notify_one();
                // set to false to be able to re-calculate the fft
                m_initiate_fft = false;
            }
        }
        // tackle rest if chunk < internal size and while has been interrupted by transform flag
        if (steps_needed == 1)
        {
            while (daw_chunk_write_index < t_size)
            {
                fill_in_and_out_buffers();
                m_circular_buffer_ptr->advance();
            }
        }
    }

    void reset(const double sampling_freq) noexcept
    {
        m_sampling_freq = sampling_freq;
        m_previous_sample = 0;
    }

    /// @note this is only needed for testing purposes, could be deletet later on.
    [[nodiscard]] [[maybe_unused]] size_t ring_buffer_index() const noexcept
    {
        return m_circular_buffer_ptr->current_index();
    }

  private:
    // Juce uses double as sample frequency, since I'll use the framework for deployment I'll use double too.
    double m_sampling_freq = 44100.0;
    // buffers
    std::shared_ptr<CircularSampleBuffer<T, FFT_SIZE, WAVETABLE_SIZE>> m_circular_buffer_ptr;
    // oscillators (needed to receive their output)
    std::shared_ptr<ResynthOscs<T, WAVETABLE_SIZE, FFT_SIZE>> m_resynth_oscs_ptr;
    // synchronization and behavioural
    std::shared_ptr<SyncPrimitives> m_calculation_sp_ptr;
    bool m_initiate_fft = false;
    // LPF
    T m_alpha = 1;
    T m_previous_sample = 0;
    float m_gain = 1.0;
};

} // namespace LBTS::Spectral
