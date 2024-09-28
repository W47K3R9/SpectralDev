#pragma once

namespace LBTS::Spectral
{
/// @note: The sampling rate of a project can change at runtime.
///     It's not recommended to use an arbitrary large amount of samples nonetheless.
///     For now, I'll set the maximum to 500ms with 48 kHz. Furthermore, a sufficient amount of samples is needed for
///     an accurate transformation. Frequency resolution = sampling freq (fs) / number of samples (n)
///     16 samples leads to a miserable 3 kHz resolution with 48 kHz fs but let's see how it sounds :D
/// @todo: Run performance test to see how long an FFT takes for different amounts of samples.
template <unsigned max_num_of_samples = 24000, unsigned min_num_of_samples = 16>
consteval bool is_power_of_two(unsigned const number)
{
    return number <= max_num_of_samples && number >= min_num_of_samples && number != 0 && (number & (number - 1)) == 0;
}

/// @brief: This struct is mainly used as a security check for being sure to initialize values according to a real
///     power of two. This is a compile-time check and NOT intended to be an object.
template <unsigned pot>
    requires(is_power_of_two(pot))
struct POTSamples
{
    POTSamples() = delete;
    POTSamples(const POTSamples&) = delete;
    POTSamples(POTSamples&&) = delete;
    POTSamples& operator=(const POTSamples&) = delete;
    POTSamples& operator=(POTSamples&&) = delete;
    ~POTSamples() = delete;
    static constexpr unsigned value = pot;
};

} // namespace LBTS::Spectral
