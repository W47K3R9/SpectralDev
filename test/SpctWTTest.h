/**
 * Author: Lucas Scheidt
 * Date: 29.12.24
 *
 * Description:
 */

#pragma once
#include "SpctWavetables.h"
#include <cassert>

namespace LBTS::Spectral
{
inline void test_wavetable_creation()
{
    const SineWT<double, 256> m_sine_wt{};
    assert(m_sine_wt[1] > 0);
    assert(m_sine_wt[125] > 0);
    assert(m_sine_wt[129] < 0);
    assert(m_sine_wt[255] < 0);
    m_sine_wt.equalize_end_and_begin();
    assert(m_sine_wt[255] == 0);

    const SquareWT<double, 256> m_square_wt{};
    assert(m_square_wt[0] == -1);
    assert(m_square_wt[127] == -1);
    assert(m_square_wt[128] == 1);

    const TriWT<double, 256> m_tri_wt{};
    assert(m_tri_wt[1] > 0);
    assert(m_tri_wt[32] >= 0.5);
    assert(m_tri_wt[127] > 0);
    assert(m_tri_wt[128] <= 0);
    assert(m_tri_wt[255] < 0);

    // ResynthOscs<double, 512, 1024> m_res_oscs{48000.0};
    // m_res_oscs.reset(44100.0);
}

} // namespace LBTS::Spectral
