#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <mutex>

namespace agro {

enum class EventLevel {
    INFO,
    WARNING,
    FAULT,
    RECOVERY,
    ROLLBACK
};

struct EventRecord {
    SimTime time;
    EventLevel level;
    std::string category;
    std::string message;

    [[nodiscard]] std::string toString(bool plain = true) const {
        std::ostringstream oss;
        oss << "[" << std::setfill('0') << std::setw(6) << time.ticks << "m] ";
        if (plain) {
            switch (level) {
                case EventLevel::INFO: oss << "[INFO] "; break;
                case EventLevel::WARNING: oss << "[WARN] "; break;
                case EventLevel::FAULT: oss << "[FAULT] "; break;
                case EventLevel::RECOVERY: oss << "[RECOVERY] "; break;
                case EventLevel::ROLLBACK: oss << "[ROLLBACK] "; break;
            }
        } else {
            // ANSI color codes
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

class EventLog {
public:
    static EventLog& instance() {
        static EventLog log;
        return log;
    }

    void append(SimTime time, EventLevel level, std::string category, std::string message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_records.push_back(EventRecord{time, level, std::move(category), std::move(message)});
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_records.clear();
    }

    [[nodiscard]] std::vector<EventRecord> getRecords() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_records;
    }

    [[nodiscard]] std::vector<std::string> getFormattedLines(bool plain = true) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> lines;
        lines.reserve(m_records.size());
        for (const auto& rec : m_records) {
            lines.push_back(rec.toString(plain));
        }
        return lines;
    }

private:
    EventLog() = default;
    mutable std::mutex m_mutex;
    std::vector<EventRecord> m_records;
};

} // namespace agro
