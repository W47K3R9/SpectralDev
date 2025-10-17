#include "test/SpctBufferManagerTest.h"
#include "test/SpctDomainSpecificTest.h"
#include "test/SpctOscillatorTest.h"
#include "test/SpctRealVoiceTest.h"
#include "test/SpctWTTest.h"

int main()
{
    using namespace LBTS::Spectral;
    test_real_voice();
    test_buffer_manager();
    test_domain_specific_functions_and_values();
    test_wavetable_creation();
    test_oscillator();
}
