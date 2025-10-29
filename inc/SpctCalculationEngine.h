/**
 * Author: Lucas Scheidt
 * Date: 13.10.25
 *
 * Description: The calculation engine manages the fft transformation. It stores a [frequency bin, amplitude] map that
 * represents the spectral representation (without phase information) of the input signal. The oscillators get tuned
 * according to that map. Since the calculation engine contains the spectral representation of the signal it will also
 * be responsible for the tuning of the oscillators.
 */
#pragma once
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include "SpctOscillatorStack.h"
#include "SpctProcessingFunctions.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
namespace LBTS::Spectral
{

template <FloatingPt T, size_t BUFFER_SIZE = BoundedPowTwo_v<size_t, 1024>>
    requires(is_bounded_pow_two(BUFFER_SIZE))
class CalculationEngine
{
  public:
    CalculationEngine(std::shared_ptr<ResynthOscs<T, BUFFER_SIZE / 4, BUFFER_SIZE>> resynth_osc_ptr,
                      std::shared_ptr<CircularSampleBuffer<T, BUFFER_SIZE>> circular_sample_buffer_ptr,
                      std::shared_ptr<SyncPrimitives> calculation_sync_primitives,
                      std::shared_ptr<SyncPrimitives> tuning_sync_primitives)
        : m_resynth_oscs_ptr{std::move(resynth_osc_ptr)},
          m_circular_sample_buffer_ptr{std::move(circular_sample_buffer_ptr)},
          m_calculation_sp_ptr{std::move(calculation_sync_primitives)},
          m_tuning_sp_ptr{std::move(tuning_sync_primitives)},
          m_fft_worker(std::thread([this] { this->fft_calculation(); })),
          m_tuning_worker(std::thread([this] { this->oscillator_tuning(); }))
    {
        prepare_to_play();
    }
    CalculationEngine(const CalculationEngine&) = delete;
    CalculationEngine(CalculationEngine&&) = delete;
    CalculationEngine& operator=(const CalculationEngine&) = delete;
    CalculationEngine& operator=(CalculationEngine&&) = delete;
    ~CalculationEngine()
    {
        m_stop_workers = true;
        // cancel waiting on cv's
        m_calculation_sp_ptr->signalling_cv.notify_all();
        m_tuning_sp_ptr->signalling_cv.notify_all();
        if (m_fft_worker.joinable())
        {
            m_fft_worker.join();
        }
        if (m_tuning_worker.joinable())
        {
            m_tuning_worker.join();
        }
    }

    /// @brief Sets detection threshold for the FFT. The amplitude of a frequency bin has to be at least as high as
    /// the...
    /// @param threshold: ... value provided here.
    /// @return void
    void set_threshold(const T threshold) noexcept { m_threshold = threshold; }

    /// @brief Sets the number of oscillators (the voices) to participate in the sound resynthesization.
    /// @param num_voices: Must be between 0 and the maximum number of oscillators available.
    /// @return void
    void set_voices(const size_t num_voices) noexcept { m_voices = std::clamp<size_t>(num_voices, 0, max_oscillators); }

    void prepare_to_play()
    {
        // Initiate first call of fft calculation by setting action_done to true. Will be reset by BufferManager
        // afterwards.
        m_calculation_sp_ptr->action_done = true;
        m_tuning_sp_ptr->action_done = true;
        {
            std::lock_guard lock{m_bin_mag_array_mtx};
            // could this cause issues due to the "int-ness" of the pure 0s?
            m_bin_mag_arr.fill({0, 0});
        }
    }

  private:
    /// @brief function that manages the FFT calculation. Gets activated by the BufferManager everytime a buffer is
    /// filled.
    void fft_calculation()
    {
        assert(m_calculation_sp_ptr != nullptr);
        assert(m_circular_sample_buffer_ptr != nullptr);
        while (!m_stop_workers)
        {
            std::unique_lock lock{m_calculation_sp_ptr->signalling_mtx};
            /// @todo consider adding a predicate.
            m_calculation_sp_ptr->signalling_cv.wait(lock);
            // omit trigger at shutdown.
            if (!m_stop_workers)
            {
                auto& fft_samples = m_circular_sample_buffer_ptr->m_out_array;
                // pass the whole array as reference to the FFT, will change the output array!
                spct_fourier_transform<T, degree_of_pow_two_value(BUFFER_SIZE)>(fft_samples, m_exponent_lut);
                // calculate the dominant magnitudes, won't change the output array
                {
                    std::lock_guard lock{m_bin_mag_array_mtx};
                    calculate_max_map<T, BUFFER_SIZE>(fft_samples, m_bin_mag_arr, m_threshold);
                }
                if (m_continuous_tuning)
                {
                    m_tuning_sp_ptr->signalling_cv.notify_all();
                }
                // Note that the action_done flag is necessary because the BufferManager fills the windowed input
                // samples into the circular output buffer. Without the flag a data race could occur during the calls of
                // spct_fourier_transform or calculate_max_map.
                m_calculation_sp_ptr->action_done = true;
            }
        }
    }

    /// @brief tunes the oscillators and gets triggered either continuously after the fft calculation or whenever the
    /// trigger manager gives the signal.
    void oscillator_tuning()
    {
        assert(m_tuning_sp_ptr != nullptr);
        assert(m_resynth_oscs_ptr != nullptr);
        /// @note Evaluate if there is a need for the action_done flag for the tuning.
        while (!m_stop_workers)
        {
            std::unique_lock lock{m_tuning_sp_ptr->signalling_mtx};
            m_tuning_sp_ptr->signalling_cv.wait(lock);
            // omit trigger at shutdown.
            if (!m_stop_workers)
            {
                std::lock_guard lock{m_bin_mag_array_mtx};
                m_resynth_oscs_ptr->tune_oscillators_to_fft(m_bin_mag_arr, m_voices);
                m_tuning_sp_ptr->action_done = true;
            }
        }
    }

    // oscillators
    std::shared_ptr<ResynthOscs<T, BUFFER_SIZE / 4, BUFFER_SIZE>> m_resynth_oscs_ptr;
    std::atomic_size_t m_voices{4};
    // fft-related
    BinMagArr<T, (BUFFER_SIZE / 2)> m_bin_mag_arr;
    std::mutex m_bin_mag_array_mtx;
    ExponentLUT<T> m_exponent_lut;
    std::shared_ptr<CircularSampleBuffer<T, BUFFER_SIZE>> m_circular_sample_buffer_ptr;
    std::atomic<T> m_threshold = min_gain_threshold<T>;
    // synchronization and behavioral
    std::shared_ptr<SyncPrimitives> m_calculation_sp_ptr;
    std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr;
    std::thread m_fft_worker;
    std::thread m_tuning_worker;
    std::atomic_bool m_stop_workers{false};
    // options
    std::atomic_bool m_continuous_tuning{true};
};
} // namespace LBTS::Spectral
