#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{
struct FxParameters
{
    OscWaveform waveform_selection;
    float filter_cutoff;
    float fft_threshold;
    float frequency_offset;
    float gain;
    uint16_t glide_steps;
    size_t voices;
    bool freeze;
    /// @todo implement clocked tuning along with options for both steady and midi clock.
};
} // namespace LBTS::Spectral
