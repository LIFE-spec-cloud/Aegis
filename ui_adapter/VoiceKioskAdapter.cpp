#include "VoiceKioskAdapter.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace agro {

namespace {
std::string normalizeBigrams(std::string text) {
    auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != std::string::npos) {
            str.replace(startPos, from.length(), to);
            startPos += to.length();
        }
    };
    replaceAll(text, "ek sau", "eksau");
    replaceAll(text, "ek so", "eksau");
    replaceAll(text, "do sau", "dosau");
    replaceAll(text, "dedh sau", "dedhsau");
    replaceAll(text, "dhai sau", "dhaisau");
    replaceAll(text, "paanch sau", "paanchsau");
    replaceAll(text, "rok do", "rokdo");
    return text;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::string normalized = normalizeBigrams(text);
    std::vector<std::string> tokens;
    std::istringstream iss(normalized);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
} // namespace

VoiceKioskAdapter::VoiceKioskAdapter(
    double confidenceThreshold,
    VernacularLexicon lexicon)
    : m_confidenceThreshold(confidenceThreshold),
      m_lexicon(std::move(lexicon)) {}

ParsedOrderResult VoiceKioskAdapter::parse(const InputEvent& event) {
    switch (event.type) {
        case InputEventType::VoiceText:
            return parseVoiceText(event);
        case InputEventType::ButtonTap:
            return parseButtonTap(event);
        case InputEventType::Dtmf:
            return parseDtmf(event);
    }
    ParsedOrderResult unknown;
    unknown.rawInput = event.payload;
    unknown.interpretationNote = "Unsupported input event type.";
    return unknown;
}

ParsedOrderResult VoiceKioskAdapter::parseVoiceText(const InputEvent& event) {
    ParsedOrderResult result;
    result.rawInput = event.payload;

    auto tokens = tokenize(event.payload);
    if (tokens.empty()) {
        result.confidence = 0.0;
        result.interpretationNote = "Empty voice transcript.";
        return result;
    }

    std::optional<double> foundQty;
    LexicalUnit foundUnit = LexicalUnit::Unknown;
    LexicalCrop foundCrop = LexicalCrop::Unknown;
    LexicalIntent foundIntent = LexicalIntent::Unknown;
    int recognizedTokens = 0;

    for (const auto& tok : tokens) {
        if (!foundQty.has_value()) {
            auto q = m_lexicon.matchQuantity(tok);
            if (q.has_value()) {
                foundQty = q;
                recognizedTokens++;
                continue;
            }
        }
        if (foundUnit == LexicalUnit::Unknown) {
            auto u = m_lexicon.matchUnit(tok);
            if (u != LexicalUnit::Unknown) {
                foundUnit = u;
                recognizedTokens++;
                continue;
            }
        }
        if (foundCrop == LexicalCrop::Unknown) {
            auto c = m_lexicon.matchCrop(tok);
            if (c != LexicalCrop::Unknown) {
                foundCrop = c;
                recognizedTokens++;
                continue;
            }
        }
        if (foundIntent == LexicalIntent::Unknown) {
            auto i = m_lexicon.matchIntent(tok);
            if (i != LexicalIntent::Unknown) {
                foundIntent = i;
                recognizedTokens++;
                continue;
            }
        }
    }

    // Calculate confidence score
    double score = 0.0;
    if (foundQty.has_value()) score += 0.30;
    if (foundCrop != LexicalCrop::Unknown) score += 0.30;
    if (foundUnit != LexicalUnit::Unknown) score += 0.20;
    if (foundIntent != LexicalIntent::Unknown) score += 0.20;

    // Slight noise penalty for extraneous unparsed tokens
    int noiseTokens = static_cast<int>(tokens.size()) - recognizedTokens;
    if (noiseTokens > 0) {
        score -= (noiseTokens * 0.04);
    }
    score = std::max(0.0, std::min(1.0, score));
    result.confidence = score;

    // Map to domain commodity
    CommodityType commType = CommodityType::Tomato;
    std::string cropName = "Tamatar (Tomato)";
    if (foundCrop == LexicalCrop::Onion) {
        commType = CommodityType::Onion;
        cropName = "Pyaaz (Onion)";
    } else if (foundCrop == LexicalCrop::Chilli) {
        commType = CommodityType::Chilli;
        cropName = "Mirch (Chilli)";
    }

    // Default intent is Sell if crop and quantity are specified
    IntentType intentType = IntentType::Sell;
    if (foundIntent == LexicalIntent::Transport) intentType = IntentType::Transport;
    else if (foundIntent == LexicalIntent::Stop) intentType = IntentType::EmergencyStop;

    double rawVal = foundQty.value_or(0.0);
    Quintals normalizedQuintals = m_lexicon.normalizeToQuintals(rawVal, foundUnit);

    CreateHarvestBatchCommand cmd;
    cmd.farmerId = event.sourceFarmer;
    cmd.commodity = commType;
    cmd.quantity = normalizedQuintals;
    cmd.intent = intentType;
    cmd.deadline = SimTime(1440); // Default 24 hours deadline

    result.command = cmd;

    // Format Confirmation Card
    // Per requirement: Low confidence yields a confirmation card; voice alone never commits.
    ConfirmationCard card;
    card.title = "KisanSetu Harvest Order Confirmation";
    card.requiresOneTapAction = true;
    card.pendingCommand = cmd;

    std::ostringstream oss;
    oss << "Kya aap " << rawVal << " ";
    switch (foundUnit) {
        case LexicalUnit::Crate: oss << "peti (crates) "; break;
        case LexicalUnit::Quintal: oss << "quintal "; break;
        case LexicalUnit::Kg: oss << "kg "; break;
        case LexicalUnit::Unknown: oss << "quintal "; break;
    }
    oss << cropName << " mandi me bechna chahte hain?";
    card.promptHinglish = oss.str();

    std::ostringstream detailsOss;
    detailsOss << "Normalized: " << normalizedQuintals.value() << " Quintals | Confidence: " 
               << static_cast<int>(score * 100.0) << "%";
    card.summaryDetails = detailsOss.str();

    result.confirmationCard = card;

    if (score < m_confidenceThreshold) {
        result.interpretationNote = "Low confidence: Confirmation Card required before committing.";
    } else {
        result.interpretationNote = "High confidence parse: Confirmation Card prepared for one-tap green tick.";
    }

    return result;
}

ParsedOrderResult VoiceKioskAdapter::parseButtonTap(const InputEvent& event) {
    ParsedOrderResult result;
    result.rawInput = event.payload;
    result.confidence = 1.0;

    std::string btn = event.payload;
    std::transform(btn.begin(), btn.end(), btn.begin(), [](unsigned char c) { return std::toupper(c); });

    if (btn == "GREEN_TICK" || btn == "CONFIRM" || btn == "HAAN" || btn == "BOL_KAR_BECHEIN") {
        ConfirmContractCommand cmd;
        cmd.farmerId = event.sourceFarmer;
        cmd.confirmed = true;
        result.command = cmd;
        result.interpretationNote = "Confirmed via one-tap green tick button.";
    } else if (btn == "RED_CROSS" || btn == "CANCEL" || btn == "REJECT") {
        RejectContractCommand cmd;
        cmd.farmerId = event.sourceFarmer;
        cmd.reason = "User rejected via cancel button.";
        result.command = cmd;
        result.interpretationNote = "Rejected via button tap.";
    } else {
        result.confidence = 0.5;
        result.interpretationNote = "Unknown button tap ID: " + event.payload;
    }

    return result;
}

ParsedOrderResult VoiceKioskAdapter::parseDtmf(const InputEvent& event) {
    ParsedOrderResult result;
    result.rawInput = event.payload;
    result.confidence = 1.0;

    if (event.payload == "1" || event.payload == "*") {
        ConfirmContractCommand cmd;
        cmd.farmerId = event.sourceFarmer;
        cmd.confirmed = true;
        result.command = cmd;
        result.interpretationNote = "Confirmed via DTMF key 1.";
    } else if (event.payload == "2" || event.payload == "#") {
        RejectContractCommand cmd;
        cmd.farmerId = event.sourceFarmer;
        cmd.reason = "User cancelled via DTMF key 2.";
        result.command = cmd;
        result.interpretationNote = "Cancelled via DTMF.";
    } else {
        result.confidence = 0.4;
        result.interpretationNote = "Unrecognized DTMF code: " + event.payload;
    }

    return result;
}

} // namespace agro
