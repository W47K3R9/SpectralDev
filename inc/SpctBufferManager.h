/**
 * The buffer size manager is the first stage of the audio plugin.
 * It collects the amount of samples specified by the ControlPanel and forwards
 * them to the FourierMap.
 */
#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{
/// @note functionality with a buffer:
/// Buffer > FFT Size : Split buffer and operate in chunks.
/// Buffer < FFT Size : Add next buffer until FFT size reached.
/// Buffer = FFT Size : Forward directly.
enum class BufferSize
{
    GREATER_THAN_DAW_SETTINGS,
    SMALLER_OR_EQUAL_TO_DAW_SETTINGS
};

class BufferManager
{
  public:
    BufferManager();
    void updateBufferSize(unsigned t_new_size);

  private:
    void fillBuffer();

  private:
    double m_sample_memory[POTSamples<16384>::value] = {};
};
} // namespace LBTS::Spectral
