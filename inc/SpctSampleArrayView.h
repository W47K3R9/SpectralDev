/**
 * Author: Lucas Scheidt
 * Date: 20.10.24
 * 
 * Description: This was an early stage implementation... it's deprecated and will probably be removed in the final
 * product.
 */

#pragma once
#include "SpctDomainSpecific.h"

namespace LBTS::Spectral{
/// @brief Allows a view on a C style array while providing begin() and end() T* iterators.
/// Note that every parameter needs to be known at compile time so this class is not suited for arrays of changing size.
template <typename T, size_t slice_size, size_t original_size = 16384>
    requires(is_bounded_pow_two(slice_size) && is_bounded_pow_two(original_size) && original_size >= slice_size)
struct StaticSampleArrayView
{
  public:
    using type = T;
    explicit StaticSampleArrayView(T* t_array_to_pass) : m_array_pointer{t_array_to_pass} {}
    ~StaticSampleArrayView() = default;
    /// @note Unsafe because there is NO range check on t_slice!
    void write_to_slice_unsafe(T* t_slice)
    {
        /// @todo in here loop unrolling may be helpful.
        for (unsigned i = 0; i < slice_size; ++i)
        {
            m_array_pointer[i + m_slice_position] = t_slice[i];
        }
    }
    void write_to_slice(std::array<T, slice_size>& t_slice)
    {
        std::copy(t_slice.begin(), t_slice.end(), this->begin());
    }
    /// @brief Hops from one slice to the next without overflowing.
    void advance() noexcept
    {
        m_slice_position += slice_size;
        m_slice_position &= original_size - 1;
    }
    /// @brief Bracket implementation with range check in sense of wrapping around max value.
    /// This will prevent a bad access BUT unexpected results may be returned if you try to access a value outside
    /// the slice size range.
    T operator[](const size_t t_position_to_read) { return m_array_pointer[t_position_to_read & slice_size - 1]; }
    T operator[](const size_t t_position_to_read) const { return m_array_pointer[t_position_to_read & slice_size - 1]; }
    T* begin() const noexcept { return m_array_pointer + m_slice_position; }
    T* end() const noexcept { return m_array_pointer + m_slice_position + slice_size; }
    [[nodiscard]] unsigned size() const noexcept { return slice_size; }

  private:
    size_t m_slice_position = 0;
    T* m_array_pointer;
};
}