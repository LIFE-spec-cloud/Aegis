#pragma once

#include <cstdint>
#include <string>
#include <compare>
#include <cmath>
#include <ostream>
#include <functional>

namespace kisansetu::contracts {

/**
 * @brief Strong type for Agent identifiers.
 */
class AgentId {
public:
    AgentId() = default;
    explicit AgentId(std::string id) : id_(std::move(id)) {}

    [[nodiscard]] const std::string& str() const noexcept { return id_; }
    [[nodiscard]] bool empty() const noexcept { return id_.empty(); }

    auto operator<=>(const AgentId&) const = default;
    bool operator==(const AgentId&) const = default;

private:
    std::string id_;
};

inline std::ostream& operator<<(std::ostream& os, const AgentId& id) {
    return os << id.str();
}

/**
 * @brief Strong type for Contract identifiers.
 */
class ContractId {
public:
    constexpr ContractId() noexcept : id_(0) {}
    constexpr explicit ContractId(uint64_t id) noexcept : id_(id) {}

    [[nodiscard]] constexpr uint64_t value() const noexcept { return id_; }
    [[nodiscard]] std::string str() const {
        return "CTR-" + std::to_string(id_);
    }

    constexpr auto operator<=>(const ContractId&) const = default;
    constexpr bool operator==(const ContractId&) const = default;

private:
    uint64_t id_{0};
};

inline std::ostream& operator<<(std::ostream& os, const ContractId& id) {
    return os << id.str();
}

/**
 * @brief Strong type for currency in integer paise (1 Rupee = 100 paise).
 */
class Money {
public:
    constexpr Money() noexcept : paise_(0) {}
    constexpr explicit Money(int64_t paise) noexcept : paise_(paise) {}

    [[nodiscard]] static constexpr Money fromPaise(int64_t p) noexcept {
        return Money(p);
    }

    [[nodiscard]] static constexpr Money fromRupees(double r) noexcept {
        return Money(static_cast<int64_t>(std::round(r * 100.0)));
    }

    [[nodiscard]] constexpr int64_t toPaise() const noexcept { return paise_; }
    [[nodiscard]] constexpr double toRupees() const noexcept {
        return static_cast<double>(paise_) / 100.0;
    }

    constexpr auto operator<=>(const Money&) const = default;
    constexpr bool operator==(const Money&) const = default;

    constexpr Money operator+(Money o) const noexcept { return Money(paise_ + o.paise_); }
    constexpr Money operator-(Money o) const noexcept { return Money(paise_ - o.paise_); }
    constexpr Money operator*(double factor) const noexcept {
        return Money(static_cast<int64_t>(std::round(static_cast<double>(paise_) * factor)));
    }
    constexpr Money operator/(double divisor) const noexcept {
        return Money(static_cast<int64_t>(std::round(static_cast<double>(paise_) / divisor)));
    }

    constexpr Money& operator+=(Money o) noexcept {
        paise_ += o.paise_;
        return *this;
    }
    constexpr Money& operator-=(Money o) noexcept {
        paise_ -= o.paise_;
        return *this;
    }

private:
    int64_t paise_{0};
};

inline std::ostream& operator<<(std::ostream& os, const Money& m) {
    return os << "₹" << m.toRupees();
}

/**
 * @brief Strong type for agricultural weight in Quintals (1 quintal = 100 kg).
 */
class Quintals {
public:
    constexpr Quintals() noexcept : value_(0.0) {}
    constexpr explicit Quintals(double val) noexcept : value_(val) {}

    [[nodiscard]] static constexpr Quintals fromKg(double kg) noexcept {
        return Quintals(kg / 100.0);
    }

    [[nodiscard]] constexpr double value() const noexcept { return value_; }
    [[nodiscard]] constexpr double toKg() const noexcept { return value_ * 100.0; }

    constexpr auto operator<=>(const Quintals&) const = default;
    constexpr bool operator==(const Quintals&) const = default;

    constexpr Quintals operator+(Quintals o) const noexcept { return Quintals(value_ + o.value_); }
    constexpr Quintals operator-(Quintals o) const noexcept { return Quintals(value_ - o.value_); }
    constexpr Quintals operator*(double factor) const noexcept { return Quintals(value_ * factor); }
    constexpr Quintals operator/(double divisor) const noexcept { return Quintals(value_ / divisor); }

    constexpr Quintals& operator+=(Quintals o) noexcept {
        value_ += o.value_;
        return *this;
    }
    constexpr Quintals& operator-=(Quintals o) noexcept {
        value_ -= o.value_;
        return *this;
    }

private:
    double value_{0.0};
};

inline std::ostream& operator<<(std::ostream& os, const Quintals& q) {
    return os << q.value() << " qtl";
}

/**
 * @brief Strong type for simulation time in discrete minutes (1 tick = 1 minute).
 */
class SimTime {
public:
    constexpr SimTime() noexcept : ticks_(0) {}
    constexpr explicit SimTime(int64_t ticks) noexcept : ticks_(ticks) {}

    [[nodiscard]] constexpr int64_t ticks() const noexcept { return ticks_; }
    [[nodiscard]] constexpr double toHours() const noexcept {
        return static_cast<double>(ticks_) / 60.0;
    }

    constexpr auto operator<=>(const SimTime&) const = default;
    constexpr bool operator==(const SimTime&) const = default;

    constexpr SimTime operator+(SimTime o) const noexcept { return SimTime(ticks_ + o.ticks_); }
    constexpr SimTime operator-(SimTime o) const noexcept { return SimTime(ticks_ - o.ticks_); }

    constexpr SimTime& operator+=(SimTime o) noexcept {
        ticks_ += o.ticks_;
        return *this;
    }
    constexpr SimTime& operator-=(SimTime o) noexcept {
        ticks_ -= o.ticks_;
        return *this;
    }

private:
    int64_t ticks_{0};
};

inline std::ostream& operator<<(std::ostream& os, const SimTime& t) {
    return os << t.ticks() << " min (" << t.toHours() << " h)";
}

/**
 * @brief Strong type for duration in Hours.
 */
class Hours {
public:
    constexpr Hours() noexcept : hours_(0.0) {}
    constexpr explicit Hours(double h) noexcept : hours_(h) {}
    constexpr explicit Hours(SimTime t) noexcept : hours_(t.toHours()) {}

    [[nodiscard]] constexpr double value() const noexcept { return hours_; }
    [[nodiscard]] constexpr SimTime toSimTime() const noexcept {
        return SimTime(static_cast<int64_t>(std::round(hours_ * 60.0)));
    }

    constexpr auto operator<=>(const Hours&) const = default;
    constexpr bool operator==(const Hours&) const = default;

    constexpr Hours operator+(Hours o) const noexcept { return Hours(hours_ + o.hours_); }
    constexpr Hours operator-(Hours o) const noexcept { return Hours(hours_ - o.hours_); }

private:
    double hours_{0.0};
};

inline std::ostream& operator<<(std::ostream& os, const Hours& h) {
    return os << h.value() << " hrs";
}

/**
 * @brief Strong type for RoadGraph node identifiers.
 */
class NodeId {
public:
    NodeId() = default;
    explicit NodeId(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const std::string& str() const noexcept { return name_; }
    [[nodiscard]] bool empty() const noexcept { return name_.empty(); }

    auto operator<=>(const NodeId&) const = default;
    bool operator==(const NodeId&) const = default;

private:
    std::string name_;
};

inline std::ostream& operator<<(std::ostream& os, const NodeId& node) {
    return os << node.str();
}

/**
 * @brief Supported agricultural commodities.
 */
enum class CommodityType {
    Tomato,
    Onion,
    Chilli
};

inline std::string toString(CommodityType type) {
    switch (type) {
        case CommodityType::Tomato: return "Tomato";
        case CommodityType::Onion:  return "Onion";
        case CommodityType::Chilli: return "Chilli";
    }
    return "Unknown";
}

/**
 * @brief Freshness grade thresholds based on quality metric Q.
 * Grade A: Q >= 0.85 (Price multiplier: 1.00)
 * Grade B: Q >= 0.65 (Price multiplier: 0.75)
 * Grade C: Q >= 0.40 (Price multiplier: 0.45)
 * Spoiled: Q <  0.40 (Price multiplier: 0.00)
 */
enum class FreshnessGrade {
    GradeA,
    GradeB,
    GradeC,
    Spoiled
};

inline std::string toString(FreshnessGrade grade) {
    switch (grade) {
        case FreshnessGrade::GradeA:  return "GradeA";
        case FreshnessGrade::GradeB:  return "GradeB";
        case FreshnessGrade::GradeC:  return "GradeC";
        case FreshnessGrade::Spoiled: return "Spoiled";
    }
    return "Unknown";
}

/**
 * @brief Physical truck operating state.
 */
enum class TruckState {
    IDLE,
    EN_ROUTE,
    STALLED,
    FAILED
};

inline std::string toString(TruckState state) {
    switch (state) {
        case TruckState::IDLE:     return "IDLE";
        case TruckState::EN_ROUTE: return "EN_ROUTE";
        case TruckState::STALLED:  return "STALLED";
        case TruckState::FAILED:   return "FAILED";
    }
    return "Unknown";
}

/**
 * @brief TransitContract state machine states.
 */
enum class ContractState {
    CREATED,
    PREPARING,
    COMMITTED,
    COMPLETED,
    COMPENSATED,
    ABORTED
};

inline std::string toString(ContractState state) {
    switch (state) {
        case ContractState::CREATED:     return "CREATED";
        case ContractState::PREPARING:   return "PREPARING";
        case ContractState::COMMITTED:   return "COMMITTED";
        case ContractState::COMPLETED:   return "COMPLETED";
        case ContractState::COMPENSATED: return "COMPENSATED";
        case ContractState::ABORTED:     return "ABORTED";
    }
    return "Unknown";
}

/**
 * @brief Message performatives for communication between agents.
 */
enum class Performative {
    TASK_ANNOUNCEMENT,
    CFP,
    PROPOSE,
    ACCEPT,
    REJECT,
    CANCEL,
    INFORM,
    FAILURE,
    TELEMETRY,
    DISTRESS_BID,
    DISTRESS_NOTICE,
    REROUTE_ORDER,
    HANDOFF_REQUEST,
    HANDOFF_ACCEPT
};

inline std::string toString(Performative perf) {
    switch (perf) {
        case Performative::TASK_ANNOUNCEMENT: return "TASK_ANNOUNCEMENT";
        case Performative::CFP:               return "CFP";
        case Performative::PROPOSE:           return "PROPOSE";
        case Performative::ACCEPT:            return "ACCEPT";
        case Performative::REJECT:            return "REJECT";
        case Performative::CANCEL:            return "CANCEL";
        case Performative::INFORM:            return "INFORM";
        case Performative::FAILURE:           return "FAILURE";
        case Performative::TELEMETRY:         return "TELEMETRY";
        case Performative::DISTRESS_BID:      return "DISTRESS_BID";
        case Performative::DISTRESS_NOTICE:   return "DISTRESS_NOTICE";
        case Performative::REROUTE_ORDER:     return "REROUTE_ORDER";
        case Performative::HANDOFF_REQUEST:   return "HANDOFF_REQUEST";
        case Performative::HANDOFF_ACCEPT:    return "HANDOFF_ACCEPT";
    }
    return "Unknown";
}

} // namespace kisansetu::contracts

// Standard hash specializations for strong types
namespace std {
    template <>
    struct hash<kisansetu::contracts::AgentId> {
        size_t operator()(const kisansetu::contracts::AgentId& id) const noexcept {
            return std::hash<std::string>{}(id.str());
        }
    };

    template <>
    struct hash<kisansetu::contracts::ContractId> {
        size_t operator()(const kisansetu::contracts::ContractId& id) const noexcept {
            return std::hash<uint64_t>{}(id.value());
        }
    };

    template <>
    struct hash<kisansetu::contracts::NodeId> {
        size_t operator()(const kisansetu::contracts::NodeId& node) const noexcept {
            return std::hash<std::string>{}(node.str());
        }
    };
}
