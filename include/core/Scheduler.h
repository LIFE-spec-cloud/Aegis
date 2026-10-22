#pragma once

#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include <queue>
#include <vector>
#include <optional>

namespace kisansetu::core {

/**
 * @brief Discrete-event simulation scheduler.
 *
 * Orders events deterministically by (simTime, sequenceNumber).
 * Sequence numbers are strictly monotonically increasing to guarantee total ordering.
 */
class Scheduler {
public:
    struct ScheduledEvent {
        contracts::SimTime time{0};
        uint64_t sequenceNumber{0};
        contracts::Envelope envelope;

        auto operator<=>(const ScheduledEvent& other) const {
            if (auto cmp = time <=> other.time; cmp != 0) {
                return cmp;
            }
            return sequenceNumber <=> other.sequenceNumber;
        }
    };

    Scheduler() = default;

    /**
     * @brief Schedule an envelope for delivery at a specific simulation time.
     */
    void schedule(contracts::SimTime time, contracts::Envelope envelope);

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief Inspect the next scheduled event without popping.
     */
    [[nodiscard]] std::optional<contracts::SimTime> nextEventTime() const noexcept;

    /**
     * @brief Pop the highest priority scheduled event (lowest simTime, lowest sequenceNumber).
     */
    ScheduledEvent pop();

    /**
     * @brief Clear all scheduled events and reset sequence counter.
     */
    void reset();

private:
    struct EventComparator {
        bool operator()(const ScheduledEvent& a, const ScheduledEvent& b) const {
            // Min-heap: lowest time first; if tied, lowest sequenceNumber first
            if (a.time != b.time) {
                return a.time > b.time;
            }
            return a.sequenceNumber > b.sequenceNumber;
        }
    };

    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, EventComparator> queue_;
    uint64_t nextSeq_{0};
};

} // namespace kisansetu::core
