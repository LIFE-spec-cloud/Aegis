#pragma once

#include "Types.h"
#include <string>
#include <optional>
#include <utility>

namespace agro {

// Move-only RAII reservation handle for Mandi Gate Slot
class SlotReservationHandle {
public:
    SlotReservationHandle() = default;
    SlotReservationHandle(std::string reservationId, AgentId mandiId, SimTime slotTime, ContractId contractId)
        : m_reservationId(std::move(reservationId)),
          m_mandiId(std::move(mandiId)),
          m_slotTime(slotTime),
          m_contractId(std::move(contractId)),
          m_valid(true) {}

    ~SlotReservationHandle() = default;

    SlotReservationHandle(const SlotReservationHandle&) = delete;
    SlotReservationHandle& operator=(const SlotReservationHandle&) = delete;

    SlotReservationHandle(SlotReservationHandle&& other) noexcept
        : m_reservationId(std::move(other.m_reservationId)),
          m_mandiId(std::move(other.m_mandiId)),
          m_slotTime(other.m_slotTime),
          m_contractId(std::move(other.m_contractId)),
          m_valid(other.m_valid) {
        other.m_valid = false;
    }

    SlotReservationHandle& operator=(SlotReservationHandle&& other) noexcept {
        if (this != &other) {
            m_reservationId = std::move(other.m_reservationId);
            m_mandiId = std::move(other.m_mandiId);
            m_slotTime = other.m_slotTime;
            m_contractId = std::move(other.m_contractId);
            m_valid = other.m_valid;
            other.m_valid = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return m_valid; }
    [[nodiscard]] const std::string& reservationId() const noexcept { return m_reservationId; }
    [[nodiscard]] const AgentId& mandiId() const noexcept { return m_mandiId; }
    [[nodiscard]] SimTime slotTime() const noexcept { return m_slotTime; }
    [[nodiscard]] const ContractId& contractId() const noexcept { return m_contractId; }

    void invalidate() noexcept { m_valid = false; }

private:
    std::string m_reservationId;
    AgentId m_mandiId;
    SimTime m_slotTime{0};
    ContractId m_contractId;
    bool m_valid{false};
};

// Move-only RAII reservation handle for Transporter Truck Lock
class TruckLockHandle {
public:
    TruckLockHandle() = default;
    TruckLockHandle(std::string lockId, AgentId truckId, ContractId contractId)
        : m_lockId(std::move(lockId)),
          m_truckId(std::move(truckId)),
          m_contractId(std::move(contractId)),
          m_valid(true) {}

    ~TruckLockHandle() = default;

    TruckLockHandle(const TruckLockHandle&) = delete;
    TruckLockHandle& operator=(const TruckLockHandle&) = delete;

    TruckLockHandle(TruckLockHandle&& other) noexcept
        : m_lockId(std::move(other.m_lockId)),
          m_truckId(std::move(other.m_truckId)),
          m_contractId(std::move(other.m_contractId)),
          m_valid(other.m_valid) {
        other.m_valid = false;
    }

    TruckLockHandle& operator=(TruckLockHandle&& other) noexcept {
        if (this != &other) {
            m_lockId = std::move(other.m_lockId);
            m_truckId = std::move(other.m_truckId);
            m_contractId = std::move(other.m_contractId);
            m_valid = other.m_valid;
            other.m_valid = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return m_valid; }
    [[nodiscard]] const std::string& lockId() const noexcept { return m_lockId; }
    [[nodiscard]] const AgentId& truckId() const noexcept { return m_truckId; }
    [[nodiscard]] const ContractId& contractId() const noexcept { return m_contractId; }

    void setTruckId(AgentId newTruckId) { m_truckId = std::move(newTruckId); }
    void invalidate() noexcept { m_valid = false; }

private:
    std::string m_lockId;
    AgentId m_truckId;
    ContractId m_contractId;
    bool m_valid{false};
};

// Move-only RAII reservation handle for Escrow Deposit
class EscrowHandle {
public:
    EscrowHandle() = default;
    EscrowHandle(std::string escrowId, AgentId payerId, Money amount, ContractId contractId)
        : m_escrowId(std::move(escrowId)),
          m_payerId(std::move(payerId)),
          m_amount(amount),
          m_contractId(std::move(contractId)),
          m_valid(true) {}

    ~EscrowHandle() = default;

    EscrowHandle(const EscrowHandle&) = delete;
    EscrowHandle& operator=(const EscrowHandle&) = delete;

    EscrowHandle(EscrowHandle&& other) noexcept
        : m_escrowId(std::move(other.m_escrowId)),
          m_payerId(std::move(other.m_payerId)),
          m_amount(other.m_amount),
          m_contractId(std::move(other.m_contractId)),
          m_valid(other.m_valid) {
        other.m_valid = false;
    }

    EscrowHandle& operator=(EscrowHandle&& other) noexcept {
        if (this != &other) {
            m_escrowId = std::move(other.m_escrowId);
            m_payerId = std::move(other.m_payerId);
            m_amount = other.m_amount;
            m_contractId = std::move(other.m_contractId);
            m_valid = other.m_valid;
            other.m_valid = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return m_valid; }
    [[nodiscard]] const std::string& escrowId() const noexcept { return m_escrowId; }
    [[nodiscard]] const AgentId& payerId() const noexcept { return m_payerId; }
    [[nodiscard]] Money amount() const noexcept { return m_amount; }
    [[nodiscard]] const ContractId& contractId() const noexcept { return m_contractId; }

    void setAmount(Money newAmount) noexcept { m_amount = newAmount; }
    void invalidate() noexcept { m_valid = false; }

private:
    std::string m_escrowId;
    AgentId m_payerId;
    Money m_amount{0};
    ContractId m_contractId;
    bool m_valid{false};
};

// Abstract Port for Mandi Gate Slot Allocation
class ISlotPort {
public:
    virtual ~ISlotPort() = default;
    [[nodiscard]] virtual std::optional<SlotReservationHandle> reserveSlot(
        const AgentId& mandiId,
        SimTime slotTime,
        const ContractId& contractId) = 0;
    [[nodiscard]] virtual bool cancelSlot(const SlotReservationHandle& handle) = 0;
};

// Abstract Port for Transporter Truck Locks
class ITruckLockPort {
public:
    virtual ~ITruckLockPort() = default;
    [[nodiscard]] virtual std::optional<TruckLockHandle> lockTruck(
        const AgentId& truckId,
        const ContractId& contractId) = 0;
    [[nodiscard]] virtual bool releaseTruck(const TruckLockHandle& handle) = 0;
    [[nodiscard]] virtual bool rebindTruck(
        TruckLockHandle& handle,
        const AgentId& newTruckId) = 0;
};

// Abstract Port for Broker Escrow Management
class IEscrowPort {
public:
    virtual ~IEscrowPort() = default;
    [[nodiscard]] virtual std::optional<EscrowHandle> holdDeposit(
        const AgentId& payerId,
        Money amount,
        const ContractId& contractId) = 0;
    [[nodiscard]] virtual bool refundEscrow(
        const EscrowHandle& handle,
        Money refundAmount) = 0;
    [[nodiscard]] virtual bool transferEscrow(
        const EscrowHandle& handle,
        const AgentId& toAgent,
        Money amount) = 0;
};

} // namespace agro
