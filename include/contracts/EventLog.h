#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <iostream>
#include <iomanip>

namespace kisansetu::contracts {

enum class EventLevel {
    INFO,
    WARNING,
    FAULT,
    RECOVERY,
    ROLLBACK
};

struct EventRecord {
    SimTime time{0};
    EventLevel level{EventLevel::INFO};
    std::string category;
    std::string message;

    [[nodiscard]] std::string toString(bool plain = true) const {
        std::ostringstream oss;
        oss << "[" << std::setfill('0') << std::setw(6) << time.ticks() << "m] ";
        if (plain) {
            switch (level) {
                case EventLevel::INFO: oss << "[INFO] "; break;
                case EventLevel::WARNING: oss << "[WARN] "; break;
                case EventLevel::FAULT: oss << "[FAULT] "; break;
                case EventLevel::RECOVERY: oss << "[RECOVERY] "; break;
                case EventLevel::ROLLBACK: oss << "[ROLLBACK] "; break;
            }
        } else {
            switch (level) {
                case EventLevel::INFO: oss << "\033[36m[INFO]\033[0m "; break;
                case EventLevel::WARNING: oss << "\033[33m[WARN]\033[0m "; break;
                case EventLevel::FAULT: oss << "\033[1;31m[FAULT]\033[0m "; break;
                case EventLevel::RECOVERY: oss << "\033[1;32m[RECOVERY]\033[0m "; break;
                case EventLevel::ROLLBACK: oss << "\033[1;35m[ROLLBACK]\033[0m "; break;
            }
        }
        oss << "[" << category << "] " << message;
        return oss.str();
    }
};

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

    static EventLog& instance() {
        static EventLog log;
        return log;
    }

    void append(EventLogEntry entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (entry.sequenceNumber == 0) {
            entry.sequenceNumber = ++nextSequence_;
        }
        entries_.push_back(std::move(entry));
    }

    void append(SimTime time, EventLevel level, std::string category, std::string message) {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.push_back(EventRecord{time, level, category, message});
        EventLogEntry entry{
            .timestamp = time,
            .sequenceNumber = ++nextSequence_,
            .eventType = std::move(category),
            .sourceAgent = AgentId("-"),
            .targetAgent = AgentId("-"),
            .contractId = ContractId(0),
            .details = std::move(message)
        };
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

    [[nodiscard]] std::vector<EventRecord> getRecords() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_;
    }

    [[nodiscard]] std::vector<std::string> getFormattedLines(bool plain = true) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> lines;
        lines.reserve(records_.size());
        for (const auto& rec : records_) {
            lines.push_back(rec.toString(plain));
        }
        return lines;
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
        records_.clear();
        nextSequence_ = 0;
    }

private:
    mutable std::mutex mutex_;
    std::vector<EventLogEntry> entries_;
    std::vector<EventRecord> records_;
    uint64_t nextSequence_{0};
};

} // namespace kisansetu::contracts
