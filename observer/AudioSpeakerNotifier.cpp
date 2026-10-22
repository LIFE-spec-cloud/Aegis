#include "AudioSpeakerNotifier.h"
#include <iostream>

namespace agro {

void AudioSpeakerNotifier::onTruckTelemetry(const TelemetryEvent& /*event*/) {
    // Normal telemetry does not need spoken voice interruption
}

void AudioSpeakerNotifier::onMandiPriceTick(const MandiPriceTickEvent& /*event*/) {
    // Routine price ticks do not interrupt
}

void AudioSpeakerNotifier::onAnomalyAlert(const AnomalyAlert& alert) {
    std::string spokenText;
    switch (alert.type) {
        case AnomalyType::HighwayStoppage:
            spokenText = "Kisan Bhai dhyan dein: Aapka truck highway jam me fansa hai. "
                         "Tamatar kharab hone se bachane ke liye truck factory bheja ja raha hai.";
            break;
        case AnomalyType::MandiGlut:
            spokenText = "Mandi update: Mandi me aamad badhne se bhaav gir gaya hai. "
                         "Aapka truck behtar bhaav ke liye dusri mandi reroute kiya ja raha hai.";
            break;
        case AnomalyType::TransporterFailure:
            spokenText = "Alert: Truck ka engine kharab hua hai. "
                         "Nikat-tam doosra truck turant maal lene aa raha hai.";
            break;
        case AnomalyType::StaleTelemetry:
            spokenText = "Warning: Truck se sampark toot gaya hai.";
            break;
    }
    announce(spokenText);
}

void AudioSpeakerNotifier::announce(const std::string& hinglishMessage) {
    m_spokenHistory.push_back(hinglishMessage);
    if (m_consoleOutput) {
        std::cout << "[AUDIO SPEAKER] \"" << hinglishMessage << "\"" << std::endl;
    }
}

} // namespace agro
