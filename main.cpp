#include "test/SpctArraySliceTest.h"
#include "test/SpctBufferManagerTest.h"
#include "test/SpctPowTwoTest.h"

int main()
{
    /// @todo Run performance test to see how long an FFT takes for different amounts of samples.
    using namespace LBTS::Spectral;
    test_array_slice();
    test_buffer_manager();
    test_domain_specific_functions_and_values();

}
