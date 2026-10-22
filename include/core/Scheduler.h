#pragma once

#include "contracts/Types.h"
#include "contracts/Envelope.h"
#include <functional>
#include <queue>
#include <vector>

namespace kisansetu::core {

/**
 * @brief Discrete-event simulation scheduler.
 * Orders events deterministically by (simTime, sequenceNumber).
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

    void schedule(contracts::SimTime time, contracts::Envelope envelope);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    ScheduledEvent pop();
    void reset();

private:
    struct EventComparator {
        bool operator()(const ScheduledEvent& a, const ScheduledEvent& b) const {
            // Min-heap: smallest time first; if tied, smallest sequenceNumber first
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
