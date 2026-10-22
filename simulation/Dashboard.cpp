#include "Dashboard.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace agro {

Dashboard::Dashboard(DashboardMode mode)
    : m_mode(mode) {}

void Dashboard::updateTruck(TruckLaneView truck) {
    auto it = std::find_if(m_trucks.begin(), m_trucks.end(), [&](const TruckLaneView& t) {
        return t.truckId == truck.truckId;
    });
    if (it != m_trucks.end()) {
        *it = std::move(truck);
    } else {
        m_trucks.push_back(std::move(truck));
    }
}

void Dashboard::updateMandi(MandiTickerView mandi) {
    auto it = std::find_if(m_mandis.begin(), m_mandis.end(), [&](const MandiTickerView& m) {
        return m.mandiId == mandi.mandiId;
    });
    if (it != m_mandis.end()) {
        *it = std::move(mandi);
    } else {
        m_mandis.push_back(std::move(mandi));
    }
}

std::string Dashboard::render() const {
    std::ostringstream oss;
    oss << renderHeader() << "\n"
        << renderTruckLanes() << "\n"
        << renderFreshnessMeters() << "\n"
        << renderMandiTickers() << "\n"
        << renderRecoveryLog() << "\n"
        << "================================================================================";
    return oss.str();
}

void Dashboard::display() const {
    std::cout << render() << std::endl;
}

std::string Dashboard::renderHeader() const {
    std::ostringstream oss;
    uint64_t totalHours = static_cast<uint64_t>(m_simTime.ticks()) / 60;
    uint64_t mins = static_cast<uint64_t>(m_simTime.ticks()) % 60;

    oss << "================================================================================\n"
        << "  KISANSETU (MANDIFLOW) DISCRETE-EVENT LOGISTICS SIMULATION\n"
        << "  Sim Clock: [" << std::setfill('0') << std::setw(5) << m_simTime.ticks() << "m | "
        << std::setw(2) << totalHours << "h:" << std::setw(2) << mins << "m] "
        << "| Seed: " << m_seed << " | Mode: " 
        << (m_mode == DashboardMode::Plain ? "PLAIN" : (m_mode == DashboardMode::NoColor ? "NO-COLOR" : "ANSI"))
        << "\n================================================================================";
    return oss.str();
}

std::string Dashboard::renderTruckLanes() const {
    std::ostringstream oss;
    oss << "[PANEL 1: TRUCK TRANSIT LANES]\n";
    if (m_trucks.empty()) {
        oss << "  No active trucks in transit.\n";
        return oss.str();
    }

    for (const auto& t : m_trucks) {
        int barWidth = 20;
        int filled = static_cast<int>((std::min(100.0, std::max(0.0, t.progressPercent)) / 100.0) * barWidth);
        
        std::string bar = "[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < filled) bar += "=";
            else if (i == filled) bar += ">";
            else bar += "-";
        }
        bar += "]";

        std::string stateStr;
        switch (t.state) {
            case TruckState::IDLE: stateStr = "IDLE"; break;
            case TruckState::EN_ROUTE: stateStr = "EN_ROUTE"; break;
            case TruckState::STALLED: stateStr = "STALLED"; break;
            case TruckState::FAILED: stateStr = "FAILED"; break;
        }

        oss << "  " << std::left << std::setw(12) << t.truckId.value 
            << " [" << std::setw(8) << stateStr << "] "
            << bar << " " << std::right << std::setw(3) << static_cast<int>(t.progressPercent) << "% "
            << "| Route: " << std::left << std::setw(18) << t.route
            << "| ETA: " << std::setw(5) << t.eta.ticks() << "m "
            << "| " << t.cargoSummary << "\n";
    }
    return oss.str();
}

std::string Dashboard::renderFreshnessMeters() const {
    std::ostringstream oss;
    oss << "[PANEL 2: CROP FRESHNESS METERS]\n";
    if (m_trucks.empty()) {
        oss << "  No active cargo monitored.\n";
        return oss.str();
    }

    for (const auto& t : m_trucks) {
        double q = t.quality;
        std::string grade;
        std::string colorCode;
        if (q >= 0.85) {
            grade = "Grade A [Taaza / Fresh]  - Full Price (1.00x)";
            colorCode = "\033[1;32m"; // Green
        } else if (q >= 0.65) {
            grade = "Grade B [Medium Fresh]   - Mandi Multiplier (0.75x)";
            colorCode = "\033[1;33m"; // Amber
        } else if (q >= 0.40) {
            grade = "Grade C [At Risk Floor]  - Low Multiplier (0.45x)";
            colorCode = "\033[33m";   // Dark Amber
        } else {
            grade = "SPOILED [Kharab]        - Mandi Reject (0.00x)";
            colorCode = "\033[1;31m"; // Red
        }

        oss << "  " << std::left << std::setw(12) << t.truckId.value << ": Q = "
            << std::fixed << std::setprecision(3) << q << " ";

        if (m_mode == DashboardMode::AnsiColor) {
            oss << colorCode << grade << "\033[0m\n";
        } else {
            oss << grade << "\n";
        }
    }
    return oss.str();
}

std::string Dashboard::renderMandiTickers() const {
    std::ostringstream oss;
    oss << "[PANEL 3: MANDI PRICE TICKERS & GATE SLOTS]\n";
    if (m_mandis.empty()) {
        oss << "  No mandis registered.\n";
        return oss.str();
    }

    for (const auto& m : m_mandis) {
        std::string trend = "==";
        if (m.currentPrice.paise > m.previousPrice.paise && m.previousPrice.paise > 0) trend = "^";
        else if (m.currentPrice.paise < m.previousPrice.paise) trend = "v";

        std::string glutStr = m.glutFlag ? "[GLUT ALERT!]" : "[SURGE NORMAL]";

        oss << "  " << std::left << std::setw(12) << m.name << " : "
            << std::right << std::setw(12) << m.currentPrice.format() << "/q "
            << trend << " ";

        if (m_mode == DashboardMode::AnsiColor) {
            if (m.glutFlag) oss << "\033[1;31m" << glutStr << "\033[0m ";
            else oss << "\033[32m" << glutStr << "\033[0m ";
        } else {
            oss << glutStr << " ";
        }

        oss << "| Slots: [" << m.occupiedSlots << "/" << m.capacity << " occupied]\n";
    }
    return oss.str();
}

std::string Dashboard::renderRecoveryLog() const {
    std::ostringstream oss;
    oss << "[PANEL 4: RECOVERY & EVENT LOG (Last 8 Events)]\n";

    auto records = EventLog::instance().getRecords();
    if (records.empty()) {
        oss << "  (Event log is currently empty)\n";
        return oss.str();
    }

    size_t startIdx = (records.size() > 8) ? (records.size() - 8) : 0;
    for (size_t i = startIdx; i < records.size(); ++i) {
        bool plain = (m_mode != DashboardMode::AnsiColor);
        oss << "  " << records[i].toString(plain) << "\n";
    }
    return oss.str();
}

} // namespace agro
