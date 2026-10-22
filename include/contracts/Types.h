#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <compare>
#include <functional>
#include <sstream>
#include <iomanip>

namespace agro {

// 1 tick = 1 simulated minute
struct SimTime {
    uint64_t ticks{0};

    constexpr SimTime() = default;
    constexpr explicit SimTime(uint64_t t) : ticks(t) {}

    [[nodiscard]] constexpr double toHours() const noexcept {
        return static_cast<double>(ticks) / 60.0;
    }

    [[nodiscard]] static constexpr SimTime fromHours(double hours) noexcept {
        return SimTime(static_cast<uint64_t>(hours * 60.0 + 0.5));
    }

    constexpr auto operator<=>(const SimTime&) const = default;

    constexpr SimTime operator+(const SimTime& rhs) const noexcept {
        return SimTime(ticks + rhs.ticks);
    }
    constexpr SimTime operator-(const SimTime& rhs) const noexcept {
        return SimTime(ticks >= rhs.ticks ? ticks - rhs.ticks : 0);
    }
    constexpr SimTime& operator+=(const SimTime& rhs) noexcept {
        ticks += rhs.ticks;
        return *this;
    }
};

// Strongly typed monetary value in integer paise (1 INR = 100 paise)
struct Money {
    int64_t paise{0};

    constexpr Money() = default;
    constexpr explicit Money(int64_t p) : paise(p) {}

    [[nodiscard]] static constexpr Money fromRupees(int64_t rupees) noexcept {
        return Money(rupees * 100);
    }

    [[nodiscard]] constexpr int64_t toRupees() const noexcept {
        return paise / 100;
    }

    [[nodiscard]] std::string format() const {
        std::ostringstream oss;
        int64_t absPaise = paise < 0 ? -paise : paise;
        if (paise < 0) oss << "-";
        oss << "INR " << (absPaise / 100) << "." 
            << std::setfill('0') << std::setw(2) << (absPaise % 100);
        return oss.str();
    }

    constexpr auto operator<=>(const Money&) const = default;

    constexpr Money operator+(const Money& rhs) const noexcept { return Money(paise + rhs.paise); }
    constexpr Money operator-(const Money& rhs) const noexcept { return Money(paise - rhs.paise); }
    constexpr Money operator*(double factor) const noexcept { 
        return Money(static_cast<int64_t>(static_cast<double>(paise) * factor + 0.5)); 
    }
    constexpr Money& operator+=(const Money& rhs) noexcept { paise += rhs.paise; return *this; }
    constexpr Money& operator-=(const Money& rhs) noexcept { paise -= rhs.paise; return *this; }
};

// Strong type for produce quantity in quintals (1 quintal = 100 kg)
struct Quintals {
    double value{0.0};

    constexpr Quintals() = default;
    constexpr explicit Quintals(double v) : value(v) {}

    constexpr auto operator<=>(const Quintals&) const = default;
    constexpr Quintals operator+(const Quintals& rhs) const noexcept { return Quintals(value + rhs.value); }
    constexpr Quintals operator-(const Quintals& rhs) const noexcept { return Quintals(value >= rhs.value ? value - rhs.value : 0.0); }
    constexpr Quintals operator*(double scale) const noexcept { return Quintals(value * scale); }
};

struct AgentId {
    std::string value;

    AgentId() = default;
    explicit AgentId(std::string_view v) : value(v) {}

    bool operator==(const AgentId& other) const noexcept { return value == other.value; }
    bool operator<(const AgentId& other) const noexcept { return value < other.value; }
};

struct ContractId {
    std::string value;

    ContractId() = default;
    explicit ContractId(std::string_view v) : value(v) {}

    bool operator==(const ContractId& other) const noexcept { return value == other.value; }
    bool operator<(const ContractId& other) const noexcept { return value < other.value; }
};

enum class CommodityType {
    Tomato,
    Onion,
    Chilli
};

enum class CropGrade {
    GradeA,   // Q >= 0.85 (Price multiplier: 1.00)
    GradeB,   // Q >= 0.65 (Price multiplier: 0.75)
    GradeC,   // Q >= 0.40 (Price multiplier: 0.45)
    Spoiled   // Q < 0.40  (Price multiplier: 0.00)
};

enum class TruckState {
    IDLE,
    EN_ROUTE,
    STALLED,
    FAILED
};

struct CropBatch {
    CommodityType commodity{CommodityType::Tomato};
    Quintals volume{0.0};
    SimTime harvestTime{0};
    double decayLambda{0.030};     // per hour
    double tempMultiplier{1.0};    // mu_temp (1.0 at <=20C, up to 1.5 at 40C)
    double qualityQ0{1.0};
    bool isRefrigerated{false};
};

} // namespace agro

namespace std {
    template<> struct hash<agro::AgentId> {
        size_t operator()(const agro::AgentId& id) const noexcept {
            return std::hash<std::string>{}(id.value);
        }
    };
    template<> struct hash<agro::ContractId> {
        size_t operator()(const agro::ContractId& id) const noexcept {
            return std::hash<std::string>{}(id.value);
        }
    };
}
