/**
 * Author: Lucas Scheidt
 * Date: 27.12.24
 *
 * Description: Functions for oscillators. First implementation will be sine, tri, square and ramp up.
 * Oscillators will be implemented as wavetable oscillators because... Well because I can and it's computationally not
 * too expensive.
 */

#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{

template <FloatingPt T, size_t N_SAMPLES>
    requires(is_bounded_pow_two(N_SAMPLES))
class WTOscillator
{
public:
    WTOscillator();
    ~WTOscillator();
    T output_value();
    void set_frequency();
    void reset();
    void choose_wavetable(const OscType& osc_type);
};

} // namespace LBTS::Spectral