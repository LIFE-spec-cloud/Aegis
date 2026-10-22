#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <iostream>

namespace kisansetu::contracts {

/**
 * @brief Represents a single discrete event in the append-only event log.
 */
struct EventLogEntry {
    SimTime timestamp{0};
    uint64_t sequenceNumber{0};
    std::string eventType;
    AgentId sourceAgent;
    AgentId targetAgent;
    ContractId contractId{0};
    std::string details;

    [[nodiscard]] std::string toLogLine() const {
        std::ostringstream oss;
        oss << "[T=" << timestamp.ticks() << "m]"
            << "[#" << sequenceNumber << "]"
            << "[" << eventType << "]"
            << "[Src=" << (sourceAgent.empty() ? "-" : sourceAgent.str()) << "]"
            << "[Dst=" << (targetAgent.empty() ? "-" : targetAgent.str()) << "]"
            << "[Ctr=" << (contractId.value() == 0 ? "-" : contractId.str()) << "] "
            << details;
        return oss.str();
    }
};

/**
 * @brief Thread-safe, append-only EventLog used by all agents, the scheduler, and tests.
 */
class EventLog {
public:
    EventLog() = default;

    void append(EventLogEntry entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (entry.sequenceNumber == 0) {
            entry.sequenceNumber = ++nextSequence_;
        }
        entries_.push_back(std::move(entry));
    }

    void log(SimTime time,
             std::string eventType,
             AgentId sourceAgent,
             AgentId targetAgent,
             ContractId contractId,
             std::string details) {
        append(EventLogEntry{
            .timestamp = time,
            .sequenceNumber = 0,
            .eventType = std::move(eventType),
            .sourceAgent = std::move(sourceAgent),
            .targetAgent = std::move(targetAgent),
            .contractId = contractId,
            .details = std::move(details)
        });
    }

    [[nodiscard]] std::vector<EventLogEntry> entries() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_;
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    [[nodiscard]] std::vector<EventLogEntry> filterByContract(ContractId cid) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<EventLogEntry> filtered;
        for (const auto& e : entries_) {
            if (e.contractId == cid) {
                filtered.push_back(e);
            }
        }
        return filtered;
    }

    [[nodiscard]] std::vector<EventLogEntry> filterByType(const std::string& type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<EventLogEntry> filtered;
        for (const auto& e : entries_) {
            if (e.eventType == type) {
                filtered.push_back(e);
            }
        }
        return filtered;
    }

    void dump(std::ostream& os) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& e : entries_) {
            os << e.toLogLine() << "\n";
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        nextSequence_ = 0;
    }

private:
    mutable std::mutex mutex_;
    std::vector<EventLogEntry> entries_;
    uint64_t nextSequence_{0};
};

} // namespace kisansetu::contracts
