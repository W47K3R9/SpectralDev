/**
 * The buffer size manager is the first stage of the audio plugin.
 * It collects the amount of samples specified by the ControlPanel and forwards
 * them to the FourierMap.
 */
#pragma once
#include "SpectralDomainSpec.h"

namespace LBTS::Spectral
{
/// @note functionality with a buffer:
/// Buffer > FFT Size : Split buffer and operate in chunks.
/// Buffer < FFT Size : Add next buffer until FFT size reached.
/// Buffer = FFT Size : Forward directly.

class BufferSizeManager
{
  public:
    void updateBufferSize(const int tNewSize);

  private:
    // I want to work with views if the Buffer is greater than FFT Size
    // and with fill and move otherwise.
    // Since the Buffersize should be editable while running I can't do
    // constexpr if to choose the right algorithm
    void fillBuffer();
};
} // namespace LBTS::Spectral
