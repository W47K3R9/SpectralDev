/**
 * Author: Lucas Scheidt
 * Date: 03.12.24
 *
 * Description: Lookup Table for complex exp numbers in the unit circle. This is needed for the FFT.
 */
#pragma once

#include "SpctDomainSpecific.h"
#include <complex>
#include <algorithm>

namespace LBTS::Spectral
{

template <FloatingPt T, size_t ELEMENTS>
    requires(is_bounded_pow_two(ELEMENTS))
struct ExponentArray
{
    ExponentArray()
    {
        // using namespace std::complex_literals;
        const T resolution = static_cast<T>(1) / ELEMENTS;
        for (size_t index = 0; index < ELEMENTS; ++index)
        {
            T rising_multiplier = std::numbers::pi_v<T> * index * resolution;
            // would have been nicer with complex_literals but then I'm limited to either float OR double...
            std::complex<T> n_one_j{0, -1};
            m_array_n[index] = std::exp(n_one_j * rising_multiplier);
        }
    }
    ExponentArray(const ExponentArray&) = delete;
    ExponentArray(ExponentArray&&) = delete;
    ExponentArray& operator=(const ExponentArray&) = delete;
    ExponentArray& operator=(const ExponentArray&&) = delete;
    ~ExponentArray() = default;

    /// @brief WARNING! To have the maximum speed available there is NO CHECK with the bracket operator.
    [[nodiscard]] std::complex<T> operator[](const size_t ndx) const { return m_array_n[ndx]; }
    [[nodiscard]] std::complex<T> at(const size_t index) const
    {
        if (index >= ELEMENTS)
        {
            throw std::out_of_range("Tried to access an omega value that is out of range of the current array!");
        }
        return m_array_n[index];
    }

  private:
    ComplexArr<T, ELEMENTS> m_array_n{};
};

template <FloatingPt T>
class ExponentLUT
{
  public:
    ExponentLUT() = default;
    ExponentLUT(const ExponentLUT& to_copy) = delete;
    ExponentLUT& operator=(const ExponentLUT& to_copy) = delete;
    ExponentLUT(ExponentLUT&& to_move) = delete;
    ExponentLUT& operator=(ExponentLUT&& to_move) = delete;
    ~ExponentLUT() = default;
    /// @note just a bit of safety, overhead probably tolerable due to sparse call of function.
    void choose_array(const size_t array_index) noexcept { m_act_array_index = std::clamp<size_t>(array_index, 0, 9); }
    std::complex<T> operator[](const size_t index) const noexcept
    {
        switch (m_act_array_index)
        {
        case 0:
            return m_array_2p0[index];
        case 1:
            return m_array_2p1[index];
        case 2:
            return m_array_2p2[index];
        case 3:
            return m_array_2p3[index];
        case 4:
            return m_array_2p4[index];
        case 5:
            return m_array_2p5[index];
        case 6:
            return m_array_2p6[index];
        case 7:
            return m_array_2p7[index];
        case 8:
            return m_array_2p8[index];
        case 9:
            return m_array_2p9[index];
        default:
            return std::complex<T>{0};
        }
    }

  private:
    size_t m_act_array_index = 0;
    ExponentArray<T, 1> m_array_2p0{};
    ExponentArray<T, 2> m_array_2p1{};
    ExponentArray<T, 4> m_array_2p2{};
    ExponentArray<T, 8> m_array_2p3{};
    ExponentArray<T, 16> m_array_2p4{};
    ExponentArray<T, 32> m_array_2p5{};
    ExponentArray<T, 64> m_array_2p6{};
    ExponentArray<T, 128> m_array_2p7{};
    ExponentArray<T, 256> m_array_2p8{};
    ExponentArray<T, 512> m_array_2p9{};
};
} // namespace LBTS::Spectral
