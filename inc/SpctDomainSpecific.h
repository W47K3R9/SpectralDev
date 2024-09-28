/**
 * Author: Lucas Scheidt
 * Date: 28.10.2024
 *
 * Purpose: This file contains domain specific declarations of concepts and types needed for the audio plugin to
 *  implement.
 */

#pragma once
#include <algorithm>

namespace LBTS::Spectral
{
/// @note The sampling rate of a project can change at runtime.
///     It's not recommended to use an arbitrary large amount of samples nonetheless.
///     For now, I'll set the maximum to 500ms with 48 kHz. Furthermore, a sufficient amount of samples is needed for
///     an accurate transformation. Frequency resolution = sampling freq (fs) / number of samples (n)
///     16 samples leads to a miserable 3 kHz resolution with 48 kHz fs but let's see how it sounds :D
/// @todo Run performance test to see how long an FFT takes for different amounts of samples.
template <unsigned max_num_of_samples = 24000, unsigned min_num_of_samples = 16>
consteval bool is_power_of_two(unsigned const number)
{
    return number <= max_num_of_samples && number >= min_num_of_samples && number != 0 && (number & (number - 1)) == 0;
}

/// @brief This struct is mainly used as a security check for being sure to initialize values according to a real
///     power of two. This is a compile-time check and NOT intended to be an object.
template <unsigned pot>
    requires(is_power_of_two(pot))
struct POTSamples
{
    POTSamples() = delete;
    static constexpr unsigned value = pot;
};

/// @brief Allows a view on a C style array while providing begin() and end() "iterators".
///     The iterators could be implemented as a subclass if more functionality is needed.
template <typename T, unsigned slice_size, unsigned original_size = 16384>
    requires(is_power_of_two(slice_size) && is_power_of_two(original_size) && original_size >= slice_size)
struct StaticSampleArrayView
{
  public:
    using type = T;
    explicit StaticSampleArrayView(T* t_array_to_pass) : m_array_pointer{t_array_to_pass} {}
    ~StaticSampleArrayView() = default;
    
    /// @brief Unsafe because there is NO range check on t_slice!
    void write_to_slice_unsafe(T* t_slice)
    {
        for (unsigned i = 0; i < slice_size; ++i)
        {
            m_array_pointer[i + m_slice_position] = t_slice[i];
        }
    }
    
    void write_to_slice(std::array<T, slice_size>& t_slice)
    {
        /// @note would be more beautiful with an external iterator.
        std::copy(t_slice.begin(), t_slice.end(), m_array_pointer + m_slice_position);
    }
    
    /// @brief hops from one slice to the next without overflowing.
    void advance()
    {
        m_slice_position += slice_size;
        m_slice_position &= original_size - 1;
    }
    
    T* begin() const { return m_array_pointer + m_slice_position; }
    T* end() const { return m_array_pointer + m_slice_position + slice_size; }
    [[nodiscard]] unsigned size() const { return slice_size; }

  private:
    unsigned m_slice_position = 0;
    T* m_array_pointer;
};
} // namespace LBTS::Spectral
