/**
 * Author: Lucas Scheidt
 * Date: 28.12.24
 *
 * Just a simple struct representing all possible FX parameters that are tweakable from external. The plugin is intended
 * to be updated with a set of such parameters
 */
#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{
struct FxParameters
{
    OscWaveform waveform_selection;
    double filter_cutoff;
    double fft_threshold;
    double frequency_offset;
    float gain;
    uint16_t glide_steps;
    size_t voices;
    bool freeze;
    /// @todo implement clocked tuning along with options for both steady and midi clock.
};
} // namespace LBTS::Spectral
