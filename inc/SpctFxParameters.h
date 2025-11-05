/**
 * Author: Lucas Scheidt
 * Date: 27.10.25
 *
 * Just a simple struct representing all possible FX parameters that are tweakable from external. The plugin is intended
 * to be updated with a set of such parameters
 */

#pragma once
#include "SpctDomainSpecific.h"
#include <cstdint>

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
    bool continuous_tuning;
    uint16_t tune_interval_ms;
    /// @todo implement clocked tuning along with options for both steady and midi clock.
};
} // namespace LBTS::Spectral
