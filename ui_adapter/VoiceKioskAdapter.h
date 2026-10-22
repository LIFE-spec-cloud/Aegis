#pragma once

#include "../include/contracts/IOrderInputPort.h"
#include "VernacularLexicon.h"
#include <memory>
#include <string>

namespace agro {

class VoiceKioskAdapter : public IOrderInputPort {
public:
    explicit VoiceKioskAdapter(
        double confidenceThreshold = 0.80,
        VernacularLexicon lexicon = VernacularLexicon());

    [[nodiscard]] ParsedOrderResult parse(const InputEvent& event) override;

    void setConfidenceThreshold(double threshold) noexcept { m_confidenceThreshold = threshold; }
    [[nodiscard]] double confidenceThreshold() const noexcept { return m_confidenceThreshold; }

    [[nodiscard]] const VernacularLexicon& lexicon() const noexcept { return m_lexicon; }
    [[nodiscard]] VernacularLexicon& lexicon() noexcept { return m_lexicon; }

private:
    [[nodiscard]] ParsedOrderResult parseVoiceText(const InputEvent& event);
    [[nodiscard]] ParsedOrderResult parseButtonTap(const InputEvent& event);
    [[nodiscard]] ParsedOrderResult parseDtmf(const InputEvent& event);

    double m_confidenceThreshold{0.80};
    VernacularLexicon m_lexicon;
};

} // namespace agro
