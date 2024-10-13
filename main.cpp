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
    auto clip_this = clip_to_lower_pow_two(24000ull);
    clip_this = clip_to_lower_pow_two(24000ul);
    clip_this = clip_to_lower_pow_two(24u);
    unsigned long smally = 900000;
    clip_this = clip_to_lower_bounded_pow_two(smally);
    std::cout << clip_this << std::endl;
}
