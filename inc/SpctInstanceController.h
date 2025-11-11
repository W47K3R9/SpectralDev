#pragma once
#include "SpctBufferManager.h"
#include "SpctCalculationEngine.h"
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctFxParameters.h"
#include "SpctResyncOscsillators.h"
#include "SpctTriggerManager.h"
#include <cstddef>
#include <memory>
namespace LBTS::Spectral
{

constexpr size_t FFT_SIZE = BoundedPowTwo_v<size_t, 1024>;
constexpr size_t WT_SIZE = BoundedPowTwo_v<size_t, 256>;

// This is specific to this file.
namespace
{
template <FloatingPt T>
using Oscs = ResynthOscs<T, WT_SIZE, FFT_SIZE>;
template <FloatingPt T>
using CircularBuffer = CircularSampleBuffer<T, FFT_SIZE, WT_SIZE>;
} // namespace

template <FloatingPt T>
class InstanceController
{
  public:
    InstanceController()
        : m_fft_sp_ptr{std::make_shared<SyncPrimitives>()},
          m_tuning_sp_ptr{std::make_shared<SyncPrimitives>()},
          m_circular_buffer_ptr{std::make_shared<CircularBuffer<T>>()},
          m_resynth_oscs_ptr{std::make_shared<Oscs<T>>(m_sampling_freq)},
          m_buff_man{m_sampling_freq, m_circular_buffer_ptr, m_resynth_oscs_ptr, m_fft_sp_ptr},
          m_calculation_engine{m_resynth_oscs_ptr, m_circular_buffer_ptr, m_fft_sp_ptr, m_tuning_sp_ptr},
          m_trigger_manager{m_tuning_sp_ptr}
    {}

    explicit InstanceController(double sampling_freq)
        : m_sampling_freq{sampling_freq},
          m_fft_sp_ptr{std::make_shared<SyncPrimitives>()},
          m_tuning_sp_ptr{std::make_shared<SyncPrimitives>()},
          m_circular_buffer_ptr{std::make_shared<CircularBuffer<T>>()},
          m_resynth_oscs_ptr{std::make_shared<Oscs<T>>(m_sampling_freq)},
          m_buff_man{m_sampling_freq, m_circular_buffer_ptr, m_resynth_oscs_ptr, m_fft_sp_ptr},
          m_calculation_engine{m_resynth_oscs_ptr, m_circular_buffer_ptr, m_fft_sp_ptr, m_tuning_sp_ptr},
          m_trigger_manager{m_tuning_sp_ptr}
    {}

    InstanceController(const InstanceController&) = delete;
    InstanceController(InstanceController&&) = delete;
    InstanceController& operator=(const InstanceController&) = delete;
    InstanceController& operator=(InstanceController&&) = delete;
    ~InstanceController() = default;

    /// @brief Will manage the configurable parameters of it's private members.
    /// @param params: Struct containing all parameters.
    /// @return void
    void update_parameters(const FxParameters& params)
    {
        m_resynth_oscs_ptr->select_waveform(params.waveform_selection);
        m_resynth_oscs_ptr->set_glide_steps(params.glide_steps);
        m_resynth_oscs_ptr->set_frequency_offset(params.frequency_offset);
        m_calculation_engine.set_voices(params.voices);
        m_calculation_engine.set_threshold(params.fft_threshold);
        m_calculation_engine.set_freeze(params.freeze);
        m_buff_man.set_cutoff(params.filter_cutoff);
        m_buff_man.set_gain(params.gain);
        m_trigger_manager.set_trigger_interval(params.tune_interval_ms);
        m_trigger_manager.set_triggered_tuning_behaviour(params.continuous_tuning);
    }

    /// @brief Main processing function, called repeatedly during playback.
    /// @param samples: Pointer to the sample array.
    /// @param chunk_size: Array length.
    /// @return void
    void process_daw_chunk(T* samples, size_t chunk_size) { m_buff_man.process_daw_chunk(samples, chunk_size); }

    /// @brief Get's called before playback (and thus before process_daw_chunk). Will reset all buffers and mute the
    /// oscillators.
    /// @param sampling_freq: The sampling frequency set inside the host (DAW).
    /// @return void
    void prepare_to_play(double sampling_freq)
    {
        m_sampling_freq = sampling_freq;
        m_circular_buffer_ptr->clear_arrays();
    }

    /// @brief Will reset the BufferManager, the ResynthOscillators and the CircularBuffer.
    /// @return void
    void reset()
    {
        m_circular_buffer_ptr->clear_arrays();
        m_resynth_oscs_ptr->reset(m_sampling_freq);
        m_buff_man.reset(m_sampling_freq);
        m_calculation_engine.reset();
    }

  private:
    double m_sampling_freq = 44100.0;
    std::shared_ptr<SyncPrimitives> m_fft_sp_ptr;
    std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr;
    std::shared_ptr<CircularBuffer<T>> m_circular_buffer_ptr;
    std::shared_ptr<Oscs<T>> m_resynth_oscs_ptr;
    BufferManager<T, FFT_SIZE, WT_SIZE> m_buff_man;
    CalculationEngine<T, FFT_SIZE, WT_SIZE> m_calculation_engine;
    TriggerManager m_trigger_manager;
};
} // namespace LBTS::Spectral
