#pragma once

#include "../include/contracts/Types.h"
#include <string>
#include <sstream>
#include <iomanip>

namespace agro {

enum class ProduceCardColor {
    Green, // Grade A: Fresh / Sell now
    Amber, // Grade B/C: At risk / Dispatch now
    Red    // Spoiled: Spoiled / Divert to processor
};

struct ProduceStatusCard {
    ProduceCardColor color{ProduceCardColor::Green};
    CropGrade grade{CropGrade::GradeA};
    double quality{1.0};
    std::string title;
    std::string adviceHinglish;
    std::string details;

    [[nodiscard]] static ProduceStatusCard fromQuality(CommodityType crop, double qualityQ) {
        ProduceStatusCard card;
        card.quality = qualityQ;

        std::string cropName = (crop == CommodityType::Tomato) ? "Tamatar" :
                               (crop == CommodityType::Onion)  ? "Pyaaz" : "Mirch";

        if (qualityQ >= 0.85) {
            card.color = ProduceCardColor::Green;
            card.grade = CropGrade::GradeA;
            card.title = cropName + " [Grade A - Taaza / Fresh]";
            card.adviceHinglish = "Bhaav sabse behtar milega. Mandi me bechein (Top Price).";
        } else if (qualityQ >= 0.40) {
            card.color = ProduceCardColor::Amber;
            card.grade = (qualityQ >= 0.65) ? CropGrade::GradeB : CropGrade::GradeC;
            card.title = cropName + ((card.grade == CropGrade::GradeB) ? " [Grade B - At Risk]" : " [Grade C - Low Grade]");
            card.adviceHinglish = "Khatra hai! Quality gir rahi hai, turant dispatch karein.";
        } else {
            card.color = ProduceCardColor::Red;
            card.grade = CropGrade::Spoiled;
            card.title = cropName + " [SPOILDED - Kharab]";
            card.adviceHinglish = "Mandi me reject hoga. Processing factory (ketchup) bhejein.";
        }

        std::ostringstream oss;
        oss << "Q: " << std::fixed << std::setprecision(2) << qualityQ 
            << " | Multiplier: ";
        switch (card.grade) {
            case CropGrade::GradeA: oss << "1.00x"; break;
            case CropGrade::GradeB: oss << "0.75x"; break;
            case CropGrade::GradeC: oss << "0.45x"; break;
            case CropGrade::Spoiled: oss << "0.00x (Process only)"; break;
        }
        card.details = oss.str();
        return card;
    }

    [[nodiscard]] std::string format(bool plain = true) const {
        std::ostringstream oss;
        if (!plain) {
            if (color == ProduceCardColor::Green) oss << "\033[1;32m";
            else if (color == ProduceCardColor::Amber) oss << "\033[1;33m";
            else oss << "\033[1;31m";
        }

        oss << "+------------------------------------------+\n"
            << "| " << std::left << std::setw(40) << title << " |\n"
            << "| " << std::left << std::setw(40) << details << " |\n"
            << "| " << std::left << std::setw(40) << adviceHinglish << " |\n"
            << "+------------------------------------------+";

        if (!plain) oss << "\033[0m";
        return oss.str();
    }
};

} // namespace agro
