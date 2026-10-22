#pragma once

#include "../include/contracts/Types.h"
#include "../include/contracts/ITelemetry.h"
#include "../include/contracts/EventLog.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

namespace agro {

enum class DashboardMode {
    AnsiColor,
    NoColor,
    Plain
};

struct TruckLaneView {
    AgentId truckId;
    std::string route;
    double progressPercent{0.0};
    TruckState state{TruckState::IDLE};
    std::string cargoSummary;
    SimTime eta{0};
    double quality{1.0};
};

struct MandiTickerView {
    AgentId mandiId;
    std::string name;
    Money currentPrice;
    Money previousPrice;
    bool glutFlag{false};
    uint32_t occupiedSlots{0};
    uint32_t capacity{4};
};

class Dashboard {
public:
    explicit Dashboard(DashboardMode mode = DashboardMode::AnsiColor);

    void setMode(DashboardMode mode) noexcept { m_mode = mode; }
    [[nodiscard]] DashboardMode mode() const noexcept { return m_mode; }

    void setSimClock(SimTime time, uint64_t seed) noexcept {
        m_simTime = time;
        m_seed = seed;
    }

    void updateTruck(TruckLaneView truck);
    void updateMandi(MandiTickerView mandi);

    // Renders the entire dashboard as a string
    [[nodiscard]] std::string render() const;

    // Print to stdout
    void display() const;

private:
    [[nodiscard]] std::string renderHeader() const;
    [[nodiscard]] std::string renderTruckLanes() const;
    [[nodiscard]] std::string renderFreshnessMeters() const;
    [[nodiscard]] std::string renderMandiTickers() const;
    [[nodiscard]] std::string renderRecoveryLog() const;

    DashboardMode m_mode{DashboardMode::AnsiColor};
    SimTime m_simTime{0};
    uint64_t m_seed{42};

    std::vector<TruckLaneView> m_trucks;
    std::vector<MandiTickerView> m_mandis;
};

} // namespace agro
