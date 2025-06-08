/**
 * The buffer size manager is the first stage of the audio plugin.
 * It collects the amount of samples specified by the ControlPanel and forwards
 * them to the FourierMap.
 */
#pragma once
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include "SpctOscillatorStack.h"
#include "SpctProcessingFunctions.h"
#include <cmath>
#include <future>

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
    BufferManager() { initiate_worker(); };
    explicit BufferManager(const double sampling_freq) : m_sampling_freq{sampling_freq}, m_oscillators{sampling_freq}
    {
        initiate_worker();
    }
    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;
    BufferManager(BufferManager&&) = delete;
    BufferManager& operator=(BufferManager&&) = delete;
    ~BufferManager()
    {
        m_stop_worker.store(true);
        m_fft_sync_cv.notify_one();
        if (m_worker.joinable())
        {
            m_worker.join();
        }
    }

    /// @brief main processing is done in here since Juce works with C-style arrays this takes in T* as first address
    /// of the array.
    void process_daw_chunk(T* daw_chunk, const size_t t_size, const T threshold = 1)
    {
        // in case the daw buffer is greater then the internal one, more steps are needed.
        // The narrowing conversion is WANTED!
        const size_t steps_needed = t_size > m_buffer_size ? std::ceil(static_cast<float>(t_size) / m_buffer_size)
                                                           : 1; // NOLINT(*-narrowing-conversions)
        size_t daw_chunk_write_index = 0;
        for (size_t step = 0; step < steps_needed; ++step)
        {
            // in case that chunk > internal size this while loop takes care of the complete filling porcess correctly
            while (!m_initiate_fft && daw_chunk_write_index < t_size)
            {
                m_ring_buffer.fill_input(daw_chunk[daw_chunk_write_index]);
                daw_chunk[daw_chunk_write_index] = m_oscillators.receive_output();
                // equicalent to m_initiate_fft.store(...);
                m_initiate_fft = m_ring_buffer.advance();
                ++daw_chunk_write_index;
            }
            if (m_initiate_fft && (m_act_interval == m_transform_interval))
            {
                m_ring_buffer.copy_to_output();
                m_fft_sync_cv.notify_one();
                m_act_interval = 0;
                // set to false to be able to re-calculate the fft
                m_initiate_fft.store(false);
            }
            /// @todo REWORK
            else if (m_initiate_fft && m_act_interval != m_transform_interval)
            {
                ++m_act_interval;
            }
        }
        // tackle rest if chunk < internal size and while has been interrupted by transform flag
        if (steps_needed == 1)
        {
            while (daw_chunk_write_index < t_size)
            {
                m_ring_buffer.fill_input(daw_chunk[daw_chunk_write_index]);
                daw_chunk[daw_chunk_write_index] = m_oscillators.receive_output();
                m_ring_buffer.advance();
                ++daw_chunk_write_index;
            }
        }
    }

    void reset(const double sampling_freq) noexcept
    {
        m_sampling_freq = sampling_freq;
        m_ring_buffer.reset_buffers();
        m_oscillators.reset(sampling_freq);
    }

    void select_osc_waveform(const OscWaveform& osc_waveform) noexcept { m_oscillators.select_waveform(osc_waveform); }

    /// @note this is only needed for testing purposes, could be deletet later on.
    [[nodiscard]] size_t ring_buffer_index() const noexcept { return m_ring_buffer.current_index(); }

  private:
    void fft_and_tuning(const T threshold)
    {
        while (!m_stop_worker)
        {
            std::unique_lock lock{m_fft_sync_mtx};
            /// @todo consider adding a predicate.
            m_fft_sync_cv.wait(lock);
            // omit trigger at shutdown.
            if (!m_stop_worker)
            {
                // pass the whole array as reference to the FFT, will change the input array!
                spct_fourier_transform<T, degree_of_pow_two_value(BUFFER_SIZE)>(m_ring_buffer.m_out_array,
                                                                                m_exponent_lut);
                // calculate the dominant magnitudes, won't change the input array
                m_valid_entries =
                    calculate_max_map<T, BUFFER_SIZE>(m_ring_buffer.m_out_array, m_bin_mag_arr, threshold);
                m_valid_entries = std::clamp<size_t>(m_valid_entries, 0, max_oscillators);
                m_oscillators.tune_oscillators_to_fft(m_bin_mag_arr, m_valid_entries);
            }
        }
    }

    void initiate_worker()
    {
        m_initiate_fft.store(false);
        m_stop_worker.store(false);
        m_worker = std::thread([this] { this->fft_and_tuning(1.0); });
    }

  private:
    CircularSampleBuffer<T, BUFFER_SIZE> m_ring_buffer{};
    size_t m_buffer_size = m_ring_buffer.size();
    ExponentLUT<T> m_exponent_lut{};
    BinMagArr<T, (BUFFER_SIZE >> 1)> m_bin_mag_arr;
    size_t m_valid_entries = 0;
    // Juce uses double as sample frequency, since I'll use the framework for deployment I'll use double too.
    double m_sampling_freq = 44100.0;
    ResynthOscs<T, BoundedPowTwo_v<size_t, 512>, BUFFER_SIZE> m_oscillators{m_sampling_freq};

    std::mutex m_fft_sync_mtx;
    std::condition_variable m_fft_sync_cv;
    std::atomic_bool m_initiate_fft;

    std::atomic_bool m_stop_worker;
    std::thread m_worker;
    // Skip every n_th transformation...
    /// @todo this MUST be reworked somehow.
    const unsigned int m_transform_interval = 0;
    unsigned int m_act_interval = 0;
};

} // namespace LBTS::Spectral
