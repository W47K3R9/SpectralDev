/**
 * Author: Lucas Scheidt
 * Date: 28.12.24
 *
 * Description: Wavetables to choose from
 */

#pragma once

#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{

template <FloatingPt T, size_t N_SAMPLES>
    requires(is_bounded_pow_two(N_SAMPLES))
struct WaveTable
{
    // default wavetable
    WaveTable() { m_wavetable.fill(0); }
    // create entries of a wavetable according to a function.
    WaveTable(std::function<T(T)> function);

    std::array<T, N_SAMPLES> m_wavetable {};
    T operator[](size_t index) const noexcept { return m_wavetable[index]; }

};

} // namespace LBTS::Spectral