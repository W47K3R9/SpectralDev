/**
 * Author: Lucas Scheidt
 * Date: 13.10.25
 *
 * Description: The calculation engine manages the fft transformation. It stores a [frequency bin, amplitude] map that
 * represents the spectral representation (without phase information) of the input signal. The oscillators get tuned
 * according to that map. Since the calculation engine contains the spectral representation of the signal it will also
 * be responsible for the tuning of the oscillators.
 */
#pragma once
#include "SpctDomainSpecific.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
namespace LBTS::Spectral
{
class TriggerManager
{

  public:
    TriggerManager(const TriggerManager&) = delete;
    TriggerManager(TriggerManager&&) = delete;
    TriggerManager& operator=(const TriggerManager&) = delete;
    TriggerManager& operator=(TriggerManager&&) = delete;
    explicit TriggerManager(std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr)
        : m_tuning_sp_ptr(std::move(m_tuning_sp_ptr)),
          m_trigger_worker([this] { triggered_tuning_worker(); })
    {}
    ~TriggerManager()
    {
        m_stop_workers = true;
        m_worker_sps.signalling_cv.notify_one();
        if (m_trigger_worker.joinable())
        {
            m_trigger_worker.join();
        }
    }

    /// @brief The trigger manager only affects the plugin if continuous tuning (the passed boolean) is false.
    /// @param continuous_tuning: A switch determining if the trigger manager or the calculation engine initiates the
    /// tuning of the oscillators.
    /// @return void
    void set_triggered_tuning_behaviour(bool continuous_tuning) noexcept
    {
        m_tuning_sp_ptr->common_ondition = continuous_tuning;
    }

    void set_trigger_interval(uint16_t time_in_ms)
    {
        const auto clamped_time_in_ms = std::clamp<uint16_t>(time_in_ms, 1, 5000);
        m_tuning_interval = std::chrono::milliseconds(clamped_time_in_ms);
    }

    void triggered_tuning_worker()
    {
        while (!m_stop_workers)
        {
            std::unique_lock lock{m_worker_sps.signalling_mtx};
            m_worker_sps.signalling_cv.wait_for(lock, m_tuning_interval);
            // m_stop_workers could have become true during wait, in that case signalling is unnecessary.
            // common_condition will be used: true -> continuous tuning, false -> triggered behaviour.
            if (!m_tuning_sp_ptr->common_ondition && !m_stop_workers)
            {
                m_tuning_sp_ptr->signalling_cv.notify_one();
            }
        }
    }

  private:
    std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr;
    SyncPrimitives m_worker_sps;
    std::thread m_trigger_worker;
    std::atomic_bool m_stop_workers{false};
    std::chrono::milliseconds m_tuning_interval{500};
};
} // namespace LBTS::Spectral
