#pragma once

#include "../include/contracts/Types.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace agro {

enum class LexicalUnit {
    Crate,
    Quintal,
    Kg,
    Unknown
};

enum class LexicalCrop {
    Tomato,
    Onion,
    Chilli,
    Unknown
};

enum class LexicalIntent {
    Sell,
    Transport,
    Stop,
    Confirm,
    Unknown
};

class VernacularLexicon {
public:
    VernacularLexicon();

    [[nodiscard]] std::optional<double> matchQuantity(const std::string& token) const;
    [[nodiscard]] LexicalUnit matchUnit(const std::string& token) const;
    [[nodiscard]] LexicalCrop matchCrop(const std::string& token) const;
    [[nodiscard]] LexicalIntent matchIntent(const std::string& token) const;

    // Convert raw quantity + unit to normalized Quintals
    [[nodiscard]] Quintals normalizeToQuintals(double rawQty, LexicalUnit unit) const;

    void setCrateWeightKg(double kg) noexcept { m_crateWeightKg = kg; }
    [[nodiscard]] double crateWeightKg() const noexcept { return m_crateWeightKg; }

private:
    void initializeDictionaries();

    double m_crateWeightKg{25.0}; // 1 crate = 25 kg = 0.25 quintals

    std::unordered_map<std::string, double> m_numberWords;
    std::unordered_map<std::string, LexicalUnit> m_unitWords;
    std::unordered_map<std::string, LexicalCrop> m_cropWords;
    std::unordered_map<std::string, LexicalIntent> m_intentWords;
};

} // namespace agro
