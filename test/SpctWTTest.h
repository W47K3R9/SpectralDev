/**
 * Author: Lucas Scheidt
 * Date: 29.12.24
 *
 * Description:
 */

#pragma once
#include "SpctWavetables.h"

namespace LBTS::Spectral
{
inline void test_wavetable_creation()
{
    const SineWT<double, 256> m_sine_wt{};
    assert(m_sine_wt[1] > 0);
    assert(m_sine_wt[125] > 0);
    assert(m_sine_wt[129] < 0);
    assert(m_sine_wt[255] < 0);

    const SquareWT<double, 256> m_square_wt{};
    assert(m_square_wt[0] == 0);
    assert(m_square_wt[127] == 0);
    assert(m_square_wt[128] == 1);

    const TriWT<double, 256> m_tri_wt{};
    assert(m_tri_wt[1] > 0);
    assert(m_tri_wt[32] >= 0.5);
    assert(m_tri_wt[127] > 0);
    assert(m_tri_wt[128] <= 0);
    assert(m_tri_wt[255] < 0);
}

} // namespace LBTS::Spectral