/**
 * Author: Lucas Scheidt
 * Date: 28.12.24
 *
 * Description: Wavetables to choose from. The first implementation only creates raw wavetables without regard to
 * the frequency they will later be used.
 *
 * @todo avoid aliasing:
 * wavetables shall be created with a frequency-dependent number of overtones via fourier series calculation.
 */

#pragma once

#include "SpctDomainSpecific.h"

namespace LBTS::Spectral
{

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
struct WaveTable
{
    // default wavetable
    WaveTable() { m_wavetable.fill(0); }
    // fill output with one cycle of the periodic function
    explicit WaveTable(std::function<T(T)> periodic_fn)
    {
        const T resolution = static_cast<T>(1) / WT_SIZE;
        // for (auto& element : m_wavetable)
        for (size_t index = 0; index < WT_SIZE; ++index)
        {
            m_wavetable[index] = periodic_fn(two_pi<T> * index * resolution);
        }
    }
    ~WaveTable() = default;
    // read only access.
    // without range check
    T operator[](const size_t index) const { return m_wavetable[index]; }
    // with range check
    T at(size_t index) const { return m_wavetable.at(index); }
    auto begin() noexcept { return m_wavetable.begin(); }
    auto cbegin() const noexcept { return m_wavetable.cbegin(); }
    auto end() noexcept { return m_wavetable.end(); }
    auto cend() const noexcept { return m_wavetable.cend(); }

  private:
    std::array<T, WT_SIZE> m_wavetable{};
};

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
struct SineWT : public WaveTable<T, WT_SIZE>
{
    SineWT() : WaveTable<T, WT_SIZE>([](T value) -> T { return std::sin<T>(value); }) {}
};

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
struct SquareWT : public WaveTable<T, WT_SIZE>
{
    SquareWT()
        : WaveTable<T, WT_SIZE>([](T value)
                                { return (value < std::numbers::pi_v<T>) ? static_cast<T>(0) : static_cast<T>(1); })
    {
    }
};

template <FloatingPt T, size_t WT_SIZE>
    requires(is_bounded_pow_two(WT_SIZE))
struct TriWT : public WaveTable<T, WT_SIZE>
{
    TriWT()
        : WaveTable<T, WT_SIZE>(
              [](T value)
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
    {
    }
};

} // namespace LBTS::Spectral