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
#include "SpctCircularBuffer.h"
#include "SpctDomainSpecific.h"
#include "SpctExponentLUT.h"
#include "SpctOscillatorStack.h"
#include "SpctProcessingFunctions.h"
#include <atomic>
#include <cassert>
#include <chrono>
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
    TriggerManager(std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr, std::thread m_trigger_worker)
        : m_tuning_sp_ptr(std::move(m_tuning_sp_ptr)),
          m_trigger_worker(std::move(m_trigger_worker))
    {}
    ~TriggerManager() = default;

    void trigger_tuning()
    {
        while (!m_stop_workers)
        {
            std::this_thread::sleep_for(m_tuning_interval);
            if (m_tuning_sp_ptr->action_done && !m_continuous_tuning)
            {
                m_tuning_sp_ptr->action_done = false;
                m_tuning_sp_ptr->signalling_cv.notify_all();
            }
        }
    }

  private:
    std::shared_ptr<SyncPrimitives> m_tuning_sp_ptr;
    std::thread m_trigger_worker;
    std::atomic_bool m_stop_workers{false};
    std::atomic_bool m_continuous_tuning{false};
    std::chrono::milliseconds m_tuning_interval{500};
};
} // namespace LBTS::Spectral
