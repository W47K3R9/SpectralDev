#include "test/SpctBufferManagerTest.h"
#include "test/SpctDomainSpecificTest.h"
#include "test/SpctRealVoiceTest.h"
#include "test/SpctWTTest.h"

int main()
{
    using namespace LBTS::Spectral;
    test_buffer_manager();
    test_domain_specific_functions_and_values();
    test_wavetable_creation();
    test_real_voice();
    // constexpr std::array<int, 5> numbers{4, 2, 4, 2, 4};
    // constexpr std::array<int, 5> mult {2, 3, 4, 5, 6};
    //  auto windowed_in = std::views::iota(static_cast<size_t>(0), numbers.size()) |
    //  std::views::transform([&](const size_t ndx) { return numbers[ndx] * mult[ndx]; });
    // std::array<int, 5> result{};
    // std::ranges::copy(windowed_in.begin(), windowed_in.end(), result.begin());
    // std::ranges::for_each(result, [](auto el){std::cout << "element: " << el << std::endl;});
}
