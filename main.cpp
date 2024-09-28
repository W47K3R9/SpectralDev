#include "inc/SpectralDomainSpec.h"
#include <format>
#include <iostream>
#include <type_traits>

int main()
{
    {
        using namespace LBTS::Spectral::Domain;
        std::array<double, POTSamples<64>::value> marr;
        auto a_mutable_pot = POTSamples<16>::value;
        std::cout << a_mutable_pot << std::endl;
    }
    std::cout << "nice";
}
