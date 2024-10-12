#include "test/SpctArraySliceTest.h"
#include "test/SpctBufferManagerTest.h"

int main()
{
    /// @todo Run performance test to see how long an FFT takes for different amounts of samples.
    using namespace LBTS::Spectral;
    test_array_slice();
    test_buffer_manager();
    auto clip_this = lower_clip_to_power_of_two(24000ull);
    clip_this = lower_clip_to_power_of_two(24000ul);
    clip_this = lower_clip_to_power_of_two(24u);
    unsigned long smally = 900000;
    clip_this = lower_clip_to_valid_power_of_two(smally);
    std::cout << clip_this << std::endl;
}
