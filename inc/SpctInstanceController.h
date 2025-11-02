#pragma once
#include "SpctBufferManager.h"
#include "SpctCalculationEngine.h"
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctFxParameters.h"
#include "SpctOscillatorStack.h"
#include "SpctTriggerManager.h"
#include <cstddef>
#include <memory>
namespace LBTS::Spectral
{

class InstanceController
{
  public:
    InstanceController();
    explicit InstanceController(double sampling_freq);
    InstanceController(const InstanceController&) = delete;
    InstanceController(InstanceController&&) = delete;
    InstanceController& operator=(const InstanceController&) = delete;
    InstanceController& operator=(InstanceController&&) = delete;
    ~InstanceController() = default;

    /// @brief Will manage the configurable parameters of it's private members.
    /// @param params: Struct containing all parameters.
    /// @return void
    void update_parameters(const FxParameters& params);

    /// @brief Main processing function, called repeatedly during playback.
    /// @param samples: Pointer to the sample array.
    /// @param chunk_size: Array length.
    /// @return void
    void process_daw_chunk(float* samples, size_t chunk_size);

    /// @brief Get's called before playback (and thus before process_daw_chunk). Will reset all buffers and mute the
    /// oscillators.
    /// @param sampling_freq: The sampling frequency set inside the host (DAW).
    /// @return void
    void prepare_to_play(double sampling_freq);

    /// @brief Will reset the BufferManager, the ResynthOscillators and the CircularBuffer.
    /// @return void
    void reset();

  private:
    double m_sampling_freq = 44100.0;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t WAVETABLE_SIZE = 256;
    std::shared_ptr<SyncPrimitives> m_fft_sp_ptr;
    std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr;
    std::shared_ptr<CircularSampleBuffer<float, BUFFER_SIZE>> m_circular_sample_buffer_ptr;
    std::shared_ptr<ResynthOscs<float, WAVETABLE_SIZE, BUFFER_SIZE>> m_resynth_oscs_ptr;
    BufferManager<float, BUFFER_SIZE> m_buff_man;
    CalculationEngine<float, BUFFER_SIZE> m_calculation_engine;
    TriggerManager m_trigger_manager;
};
} // namespace LBTS::Spectral
