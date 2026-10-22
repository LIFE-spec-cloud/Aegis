#pragma once

#include "../../include/contracts/ITelemetry.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace agro::fakes {

class FakeTelemetrySource {
public:
    void addListener(ITelemetryWatchdogListener* listener) {
        if (listener) {
            m_listeners.push_back(listener);
        }
    }

    void removeListener(ITelemetryWatchdogListener* listener) {
        m_listeners.erase(
            std::remove(m_listeners.begin(), m_listeners.end(), listener),
            m_listeners.end());
    }

    void emitTelemetry(const TelemetryEvent& event) {
        m_telemetryHistory.push_back(event);
        for (auto* listener : m_listeners) {
            listener->onTruckTelemetry(event);
        }
    }

    void emitMandiPriceTick(const MandiPriceTickEvent& event) {
        m_priceTickHistory.push_back(event);
        for (auto* listener : m_listeners) {
            listener->onMandiPriceTick(event);
        }
    }

    void emitAnomalyAlert(const AnomalyAlert& alert) {
        m_alertHistory.push_back(alert);
        for (auto* listener : m_listeners) {
            listener->onAnomalyAlert(alert);
        }
    }

    [[nodiscard]] const std::vector<TelemetryEvent>& telemetryHistory() const noexcept {
        return m_telemetryHistory;
    }

    [[nodiscard]] const std::vector<MandiPriceTickEvent>& priceTickHistory() const noexcept {
        return m_priceTickHistory;
    }

    [[nodiscard]] const std::vector<AnomalyAlert>& alertHistory() const noexcept {
        return m_alertHistory;
    }

    void clearHistory() noexcept {
        m_telemetryHistory.clear();
        m_priceTickHistory.clear();
        m_alertHistory.clear();
    }

private:
    std::vector<ITelemetryWatchdogListener*> m_listeners;
    std::vector<TelemetryEvent> m_telemetryHistory;
    std::vector<MandiPriceTickEvent> m_priceTickHistory;
    std::vector<AnomalyAlert> m_alertHistory;
};

} // namespace agro::fakes
