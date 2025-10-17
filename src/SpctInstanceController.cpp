#include "SpctInstanceController.h"
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctOscillatorStack.h"
#include <memory>
namespace LBTS::Spectral
{

using Oscs = ResynthOscs<float, 256, 1024>;
InstanceController::InstanceController(double sampling_freq)
    : m_sampling_freq{sampling_freq},
      m_fft_sp_ptr{std::make_shared<SyncPrimitives>()},
      m_tuning_sp_ptr{std::make_shared<SyncPrimitives>()},
      m_circular_sample_buffer_ptr{std::make_shared<CircularSampleBuffer<float, BUFFER_SIZE>>()},
      m_resynth_oscs_ptr{std::make_shared<Oscs>(m_sampling_freq)},
      m_buff_man{m_sampling_freq, m_circular_sample_buffer_ptr, m_resynth_oscs_ptr, m_fft_sp_ptr},
      m_calculation_engine{m_resynth_oscs_ptr, m_circular_sample_buffer_ptr, m_fft_sp_ptr, m_tuning_sp_ptr}
{}

InstanceController::InstanceController()
    : m_fft_sp_ptr{std::make_shared<SyncPrimitives>()},
      m_tuning_sp_ptr{std::make_shared<SyncPrimitives>()},
      m_circular_sample_buffer_ptr{std::make_shared<CircularSampleBuffer<float, BUFFER_SIZE>>()},
      m_resynth_oscs_ptr{std::make_shared<Oscs>(m_sampling_freq)},
      m_buff_man{m_sampling_freq, m_circular_sample_buffer_ptr, m_resynth_oscs_ptr, m_fft_sp_ptr},
      m_calculation_engine{m_resynth_oscs_ptr, m_circular_sample_buffer_ptr, m_fft_sp_ptr, m_tuning_sp_ptr}
{}

void InstanceController::update_parameters(const FxParameters& params)
{
    m_resynth_oscs_ptr->select_waveform(params.waveform_selection);
    m_resynth_oscs_ptr->set_glide_steps(params.glide_steps);
    m_calculation_engine.set_voices(params.voices);
    m_calculation_engine.set_threshold(params.fft_threshold);
    m_buff_man.set_cutoff(params.filter_cutoff);
    m_buff_man.set_gain(params.gain);
    /// params.freeze
    /// params.frequency_offset
    /// params.gain
}
void InstanceController::process_daw_chunk(float* samples, size_t chunk_size)
{
    m_buff_man.process_daw_chunk(samples, chunk_size);
}
void InstanceController::prepare_to_play() {}
void InstanceController::reset() {}

} // namespace LBTS::Spectral
