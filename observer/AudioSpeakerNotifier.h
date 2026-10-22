#pragma once

#include "../include/contracts/ITelemetry.h"
#include <vector>
#include <string>
#include <iostream>

namespace agro {

class AudioSpeakerNotifier : public ITelemetryWatchdogListener {
public:
    explicit AudioSpeakerNotifier(bool enableConsoleOutput = true)
        : m_consoleOutput(enableConsoleOutput) {}

    void onTruckTelemetry(const TelemetryEvent& event) override;
    void onMandiPriceTick(const MandiPriceTickEvent& event) override;
    void onAnomalyAlert(const AnomalyAlert& alert) override;

    // Explicit spoken announcements
    void announce(const std::string& hinglishMessage);

    [[nodiscard]] const std::vector<std::string>& transcriptHistory() const noexcept {
        return m_spokenHistory;
    }

    void clearHistory() noexcept {
        m_spokenHistory.clear();
    }

    void setConsoleOutput(bool enabled) noexcept { m_consoleOutput = enabled; }

private:
    bool m_consoleOutput{true};
    std::vector<std::string> m_spokenHistory;
};

} // namespace agro
