/**
 * Author: Lucas Scheidt
 * Date: 28.12.24
 *
 * Wavetables to choose from. The first implementation only creates raw wavetables without regard to the frequency they
 * will later be used.
 * @todo avoid aliasing:
 * wavetables shall be created with a frequency-dependent number of overtones via fourier series calculation.
 */

#pragma once

#include "SpctDomainSpecific.h"
#include <cmath>
#include <cstdint>
#include <functional>

namespace LBTS::Spectral
{

enum class FunctionType : uint8_t
{
    PERIODIC,
    WINDOWING
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct WaveTable
{
    // default wavetable
    WaveTable() { m_wavetable.fill(0); }
    // fill output according to a periodic or windowing function
    explicit WaveTable(const std::function<T(const T)> function, const FunctionType& fn_type = FunctionType::PERIODIC)
    {
        const T periodic_scalar = TWO_PI<T> * static_cast<T>(1) / WAVETABLE_SIZE;
        switch (fn_type)
        {
        case FunctionType::PERIODIC:
            for (size_t index = 0; index < WAVETABLE_SIZE; ++index)
            {
                m_wavetable[index] = function(index * periodic_scalar);
            }
            break;
        case FunctionType::WINDOWING:
            for (size_t index = 0; index < WAVETABLE_SIZE; ++index)
            {
                m_wavetable[index] = function(static_cast<T>(index));
            }
            break;
        default:
            m_wavetable.fill(0);
        }
    }
    // read only access.
    // without range check
    T operator[](const size_t index) const { return m_wavetable[index]; }
    // with range check
    T at(size_t index) const { return m_wavetable.at(index); }
    auto begin() noexcept { return m_wavetable.begin(); }
    auto cbegin() const noexcept { return m_wavetable.cbegin(); }
    auto end() noexcept { return m_wavetable.end(); }
    auto cend() const noexcept { return m_wavetable.cend(); }
    /// @brief This is needed if the wrap-around of the wavetable index happens at the next-to-last index instead of the
    /// last.
    void equalize_end_and_begin() const noexcept { m_wavetable[WAVETABLE_SIZE - 1] = m_wavetable[0]; }

  private:
    mutable std::array<T, WAVETABLE_SIZE> m_wavetable{};
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct SineWT : public WaveTable<T, WAVETABLE_SIZE>
{
    SineWT() : WaveTable<T, WAVETABLE_SIZE>([](const T value) -> T { return std::sin(value); }) {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct SquareWT : public WaveTable<T, WAVETABLE_SIZE>
{
    SquareWT()
        : WaveTable<T, WAVETABLE_SIZE>(
              [](const T value) -> T
              { return (value < std::numbers::pi_v<T>) ? static_cast<T>(-1) : static_cast<T>(1); })
    {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct SawWT : public WaveTable<T, WAVETABLE_SIZE>
{
    SawWT() : WaveTable<T, WAVETABLE_SIZE>([](const T value) -> T { return std::numbers::inv_pi_v<T> * value - 1; }) {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct TriWT : public WaveTable<T, WAVETABLE_SIZE>
{
    TriWT()
        : WaveTable<T, WAVETABLE_SIZE>(
              [](const T value) -> T
              {
                  const T half_pi = std::numbers::pi_v<T> / 2;
                  const T two_inv_pi = std::numbers::inv_pi_v<T> * 2;

                  if (value > -half_pi && value <= half_pi)
                  {
                      return two_inv_pi * value;
                  }
                  if (value > half_pi && value <= 3 * half_pi)
                  {
                      return -two_inv_pi * value + 2;
                  }
                  if (value > 3 * half_pi && value <= 2 * std::numbers::pi_v<T>)
                  {
                      return two_inv_pi * value - 4;
                  }
                  return static_cast<T>(0);
              })
    {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct HammingWindow : public WaveTable<T, WAVETABLE_SIZE>
{
    HammingWindow()
        : WaveTable<T, WAVETABLE_SIZE>([](const T value) -> T
                                       { return 0.54 - 0.46 * std::cos(TWO_PI<T> * value / (WAVETABLE_SIZE - 1)); },
                                       FunctionType::WINDOWING)
    {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct VonHannWindow : public WaveTable<T, WAVETABLE_SIZE>
{
    VonHannWindow()
        : WaveTable<T, WAVETABLE_SIZE>([](const T value) -> T
                                       { return 0.5 * (1 - std::cos(TWO_PI<T> * value / (WAVETABLE_SIZE - 1))); },
                                       FunctionType::WINDOWING)
    {}
};

template <FloatingPt T, size_t WAVETABLE_SIZE>
    requires(is_bounded_pow_two(WAVETABLE_SIZE))
struct BartlettWindow : public WaveTable<T, WAVETABLE_SIZE>
{
    BartlettWindow()
        : WaveTable<T, WAVETABLE_SIZE>(
              [](const T value) -> T
              {
                  const size_t one_less_than_wt_size = WAVETABLE_SIZE - 1;
                  const T fraction = static_cast<T>(2) / one_less_than_wt_size;
                  const T inv_fraction = static_cast<T>(1) / fraction;
                  return fraction * (inv_fraction - std::abs(value - inv_fraction));
              },
              FunctionType::WINDOWING)
    {}
};

} // namespace LBTS::Spectral
