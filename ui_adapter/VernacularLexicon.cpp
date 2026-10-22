#include "VernacularLexicon.h"
#include <algorithm>
#include <cctype>

namespace agro {

namespace {
std::string toLowerClean(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}
} // namespace

VernacularLexicon::VernacularLexicon() {
    initializeDictionaries();
}

void VernacularLexicon::initializeDictionaries() {
    // Numbers: Hindi / Hinglish words
    m_numberWords["ek"] = 1.0;
    m_numberWords["do"] = 2.0;
    m_numberWords["teen"] = 3.0;
    m_numberWords["chaar"] = 4.0;
    m_numberWords["char"] = 4.0;
    m_numberWords["paanch"] = 5.0;
    m_numberWords["panch"] = 5.0;
    m_numberWords["chhe"] = 6.0;
    m_numberWords["saat"] = 7.0;
    m_numberWords["aath"] = 8.0;
    m_numberWords["nau"] = 9.0;
    m_numberWords["das"] = 10.0;
    m_numberWords["gyaarah"] = 11.0;
    m_numberWords["baarah"] = 12.0;
    m_numberWords["pandrah"] = 15.0;
    m_numberWords["bees"] = 20.0;
    m_numberWords["pachees"] = 25.0;
    m_numberWords["tees"] = 30.0;
    m_numberWords["paintees"] = 35.0;
    m_numberWords["chaalis"] = 40.0;
    m_numberWords["chalis"] = 40.0;
    m_numberWords["pachaas"] = 50.0;
    m_numberWords["pachas"] = 50.0;
    m_numberWords["saath"] = 60.0;
    m_numberWords["sath"] = 60.0;
    m_numberWords["sattar"] = 70.0;
    m_numberWords["assi"] = 80.0;
    m_numberWords["nabbe"] = 90.0;
    m_numberWords["sau"] = 100.0;
    m_numberWords["so"] = 100.0;
    m_numberWords["eksau"] = 100.0;
    m_numberWords["dedhsau"] = 150.0;
    m_numberWords["dosau"] = 200.0;
    m_numberWords["dhaisau"] = 250.0;
    m_numberWords["paanchsau"] = 500.0;
    m_numberWords["hazaar"] = 1000.0;
    m_numberWords["hazar"] = 1000.0;

    // Units
    m_unitWords["crate"] = LexicalUnit::Crate;
    m_unitWords["crates"] = LexicalUnit::Crate;
    m_unitWords["peti"] = LexicalUnit::Crate;
    m_unitWords["petiyan"] = LexicalUnit::Crate;
    m_unitWords["petiya"] = LexicalUnit::Crate;
    m_unitWords["box"] = LexicalUnit::Crate;
    m_unitWords["boxes"] = LexicalUnit::Crate;

    m_unitWords["quintal"] = LexicalUnit::Quintal;
    m_unitWords["quintals"] = LexicalUnit::Quintal;
    m_unitWords["kintal"] = LexicalUnit::Quintal;
    m_unitWords["kattal"] = LexicalUnit::Quintal;
    m_unitWords["qtl"] = LexicalUnit::Quintal;
    m_unitWords["q"] = LexicalUnit::Quintal;

    m_unitWords["kg"] = LexicalUnit::Kg;
    m_unitWords["kilo"] = LexicalUnit::Kg;
    m_unitWords["kilogram"] = LexicalUnit::Kg;

    // Crops
    m_cropWords["tamatar"] = LexicalCrop::Tomato;
    m_cropWords["tamaatar"] = LexicalCrop::Tomato;
    m_cropWords["tamater"] = LexicalCrop::Tomato;
    m_cropWords["tomato"] = LexicalCrop::Tomato;
    m_cropWords["tomatoes"] = LexicalCrop::Tomato;

    m_cropWords["pyaaz"] = LexicalCrop::Onion;
    m_cropWords["pyaz"] = LexicalCrop::Onion;
    m_cropWords["kanda"] = LexicalCrop::Onion;
    m_cropWords["kaanda"] = LexicalCrop::Onion;
    m_cropWords["onion"] = LexicalCrop::Onion;
    m_cropWords["onions"] = LexicalCrop::Onion;

    m_cropWords["mirch"] = LexicalCrop::Chilli;
    m_cropWords["mirchi"] = LexicalCrop::Chilli;
    m_cropWords["chilli"] = LexicalCrop::Chilli;
    m_cropWords["chillies"] = LexicalCrop::Chilli;
    m_cropWords["chili"] = LexicalCrop::Chilli;

    // Intents
    m_intentWords["bechna"] = LexicalIntent::Sell;
    m_intentWords["becho"] = LexicalIntent::Sell;
    m_intentWords["bech"] = LexicalIntent::Sell;
    m_intentWords["sell"] = LexicalIntent::Sell;
    m_intentWords["bikri"] = LexicalIntent::Sell;

    m_intentWords["bhejna"] = LexicalIntent::Transport;
    m_intentWords["bhejo"] = LexicalIntent::Transport;
    m_intentWords["transport"] = LexicalIntent::Transport;
    m_intentWords["dispatch"] = LexicalIntent::Transport;

    m_intentWords["rok"] = LexicalIntent::Stop;
    m_intentWords["rokdo"] = LexicalIntent::Stop;
    m_intentWords["band"] = LexicalIntent::Stop;
    m_intentWords["cancel"] = LexicalIntent::Stop;
    m_intentWords["stop"] = LexicalIntent::Stop;

    m_intentWords["confirm"] = LexicalIntent::Confirm;
    m_intentWords["haan"] = LexicalIntent::Confirm;
    m_intentWords["theek"] = LexicalIntent::Confirm;
    m_intentWords["sahi"] = LexicalIntent::Confirm;
    m_intentWords["manzoor"] = LexicalIntent::Confirm;
    m_intentWords["yes"] = LexicalIntent::Confirm;
}

std::optional<double> VernacularLexicon::matchQuantity(const std::string& token) const {
    std::string clean = toLowerClean(token);
    if (clean.empty()) return std::nullopt;

    // Check vernacular word dictionary
    auto it = m_numberWords.find(clean);
    if (it != m_numberWords.end()) {
        return it->second;
    }

    // Check numeric digit parsing
    try {
        size_t idx = 0;
        double val = std::stod(clean, &idx);
        if (idx == clean.size()) {
            return val;
        }
    } catch (...) {
        // Not a pure number
    }

    return std::nullopt;
}

LexicalUnit VernacularLexicon::matchUnit(const std::string& token) const {
    std::string clean = toLowerClean(token);
    auto it = m_unitWords.find(clean);
    return it != m_unitWords.end() ? it->second : LexicalUnit::Unknown;
}

LexicalCrop VernacularLexicon::matchCrop(const std::string& token) const {
    std::string clean = toLowerClean(token);
    auto it = m_cropWords.find(clean);
    return it != m_cropWords.end() ? it->second : LexicalCrop::Unknown;
}

LexicalIntent VernacularLexicon::matchIntent(const std::string& token) const {
    std::string clean = toLowerClean(token);
    auto it = m_intentWords.find(clean);
    return it != m_intentWords.end() ? it->second : LexicalIntent::Unknown;
}

Quintals VernacularLexicon::normalizeToQuintals(double rawQty, LexicalUnit unit) const {
    switch (unit) {
        case LexicalUnit::Crate:
            // e.g. 50 crates * 25 kg = 1250 kg = 12.5 quintals
            return Quintals((rawQty * m_crateWeightKg) / 100.0);
        case LexicalUnit::Quintal:
            return Quintals(rawQty);
        case LexicalUnit::Kg:
            return Quintals(rawQty / 100.0);
        case LexicalUnit::Unknown:
            // Default assumes quintals if unspecified
            return Quintals(rawQty);
    }
    return Quintals(rawQty);
}

} // namespace agro
