#pragma once

#include "contracts/Envelope.h"
#include "contracts/EventLog.h"
#include "core/Agent.h"
#include "core/Scheduler.h"
#include "core/SimClock.h"
#include "core/Rng.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <optional>

namespace kisansetu::core {

/**
 * @brief Discrete-event message routing bus.
 *
 * Directs immutable Envelope messages to target agent mailboxes
 * via the deterministic Scheduler. Supports optional network jitter and packet drop.
 */
class EventBus {
public:
    explicit EventBus(Scheduler& scheduler,
                      contracts::EventLog& eventLog,
                      SimClock* clock = nullptr,
                      Rng* rng = nullptr)
        : scheduler_(scheduler),
          eventLog_(eventLog),
          clock_(clock),
          rng_(rng) {}

    void registerAgent(contracts::AgentId id, Agent* agent);
    void unregisterAgent(const contracts::AgentId& id);

    /**
     * @brief Send a message with delivery scheduled at envelope.sentAt() (or with latency).
     */
    void send(contracts::Envelope envelope);

    /**
     * @brief Send a message with an explicitly specified delivery time.
     */
    void sendAt(contracts::Envelope envelope, contracts::SimTime deliveryTime);

    /**
     * @brief Configure network simulation options (packet drop rate, latency ticks).
     */
    void setPacketDropRate(double dropRate) noexcept { dropRate_ = dropRate; }
    void setLatencyTicks(int64_t latencyTicks) noexcept { latencyTicks_ = latencyTicks; }

    /**
     * @brief Toggle multi-threaded worker dispatch for agent mailboxes.
     */
    void setParallelMode(bool enabled, size_t threadCount = 2) noexcept {
        parallelMode_ = enabled;
        threadCount_ = threadCount;
    }
    [[nodiscard]] bool isParallelMode() const noexcept { return parallelMode_; }
    [[nodiscard]] double packetDropRate() const noexcept { return dropRate_; }
    [[nodiscard]] int64_t latencyTicks() const noexcept { return latencyTicks_; }

    /**
     * @brief Dispatch the next scheduled event from the scheduler.
     * @return true if an event was dispatched, false if the queue was empty.
     */
    bool dispatchNext();

    /**
     * @brief Dispatch all scheduled events whose delivery time <= maxTime.
     * Advances SimClock if attached.
     * @return count of dispatched events.
     */
    size_t dispatchUpTo(contracts::SimTime maxTime);

    /**
     * @brief Dispatch all remaining events in the scheduler.
     * @return count of dispatched events.
     */
    size_t dispatchAll();

    [[nodiscard]] size_t registeredAgentCount() const;
    [[nodiscard]] bool hasAgent(const contracts::AgentId& id) const;

private:
    Scheduler& scheduler_;
    contracts::EventLog& eventLog_;
    SimClock* clock_{nullptr};
    Rng* rng_{nullptr};
    std::unordered_map<contracts::AgentId, Agent*> agents_;

    double dropRate_{0.0};
    int64_t latencyTicks_{0};
    bool parallelMode_{false};
    size_t threadCount_{2};
};

} // namespace kisansetu::core
