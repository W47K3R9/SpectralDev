#pragma once
#include "SpctOscillator.h"
#include <iostream>

using namespace LBTS::Spectral;

inline void test_oscillator()
{
    std::cout << "Oscillator test" << std::endl;
    WTOscillator<double, 1024> oscillator;
    // both greater (than default)
    oscillator.tune_and_set_amp(100, 0.1);
    // both lower than previous
    oscillator.tune_and_set_amp(10, 0.01);
    // freq greater
    oscillator.tune_and_set_amp(300, 0.01);
    // amp greater
    oscillator.tune_and_set_amp(100, 0.6);
    // both greater
    oscillator.tune_and_set_amp(400, 0.7);
}
