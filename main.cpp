#include "test/SpctArraySliceTest.h"
#include "test/SpctBufferManagerTest.h"
#include "test/SpctDomainSpecificTest.h"
#include "test/SpctWTTest.h"

int main()
{
    using namespace LBTS::Spectral;
    test_array_slice();
    test_buffer_manager();
    test_domain_specific_functions_and_values();
    test_wavetable_creation();
}
