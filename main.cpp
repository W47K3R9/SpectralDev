#include "test/SpctBufferManagerTest.h"
#include "test/SpctDomainSpecificTest.h"
#include "test/SpctOscillatorTest.h"
#include "test/SpctRealVoiceTest.h"
#include "test/SpctWTTest.h"
#include <cstddef>

int main()
{
    using namespace LBTS::Spectral;
    test_real_voice();
    test_buffer_manager();
    test_domain_specific_functions_and_values();
    test_wavetable_creation();
    test_oscillator();
    std::cout << "Testing clamp" << std::endl;
    size_t entry = 0;
    const double res = 44'100.0 / 1024;
    const double detune = 8.0;
    const auto freq = std::clamp((static_cast<double>(entry) + 0.5 - detune) * res, 0.0, 441'00.0 / 2);
    std::cout << "Underflow or 0 ? -> " << freq << std::endl;
}
