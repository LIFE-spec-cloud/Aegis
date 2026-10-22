#pragma once

#include "Types.h"
#include <optional>
#include <utility>

namespace kisansetu::contracts {

/**
 * @brief Move-only handle for a reserved APMC mandi gate slot.
 */
class SlotReservationHandle {
public:
    SlotReservationHandle() = default;

    SlotReservationHandle(AgentId mandiId, ContractId contractId, SimTime slotTime, uint64_t reservationId)
        : mandiId_(std::move(mandiId)),
          contractId_(contractId),
          slotTime_(slotTime),
          reservationId_(reservationId),
          active_(true) {}

    ~SlotReservationHandle() = default;

    // Move-only semantics
    SlotReservationHandle(const SlotReservationHandle&) = delete;
    SlotReservationHandle& operator=(const SlotReservationHandle&) = delete;

    SlotReservationHandle(SlotReservationHandle&& other) noexcept
        : mandiId_(std::move(other.mandiId_)),
          contractId_(other.contractId_),
          slotTime_(other.slotTime_),
          reservationId_(other.reservationId_),
          active_(other.active_) {
        other.active_ = false;
    }

    SlotReservationHandle& operator=(SlotReservationHandle&& other) noexcept {
        if (this != &other) {
            mandiId_ = std::move(other.mandiId_);
            contractId_ = other.contractId_;
            slotTime_ = other.slotTime_;
            reservationId_ = other.reservationId_;
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return active_; }
    [[nodiscard]] const AgentId& mandiId() const noexcept { return mandiId_; }
    [[nodiscard]] ContractId contractId() const noexcept { return contractId_; }
    [[nodiscard]] SimTime slotTime() const noexcept { return slotTime_; }
    [[nodiscard]] uint64_t reservationId() const noexcept { return reservationId_; }

    void invalidate() noexcept { active_ = false; }

private:
    AgentId mandiId_;
    ContractId contractId_;
    SimTime slotTime_{0};
    uint64_t reservationId_{0};
    bool active_{false};
};

/**
 * @brief Move-only handle for a locked transporter truck.
 */
class TruckLockHandle {
public:
    TruckLockHandle() = default;

    TruckLockHandle(AgentId truckId, ContractId contractId, uint64_t lockId)
        : truckId_(std::move(truckId)),
          contractId_(contractId),
          lockId_(lockId),
          active_(true) {}

    ~TruckLockHandle() = default;

    TruckLockHandle(const TruckLockHandle&) = delete;
    TruckLockHandle& operator=(const TruckLockHandle&) = delete;

    TruckLockHandle(TruckLockHandle&& other) noexcept
        : truckId_(std::move(other.truckId_)),
          contractId_(other.contractId_),
          lockId_(other.lockId_),
          active_(other.active_) {
        other.active_ = false;
    }

    TruckLockHandle& operator=(TruckLockHandle&& other) noexcept {
        if (this != &other) {
            truckId_ = std::move(other.truckId_);
            contractId_ = other.contractId_;
            lockId_ = other.lockId_;
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return active_; }
    [[nodiscard]] const AgentId& truckId() const noexcept { return truckId_; }
    [[nodiscard]] ContractId contractId() const noexcept { return contractId_; }
    [[nodiscard]] uint64_t lockId() const noexcept { return lockId_; }

    void rebind(AgentId newTruckId, uint64_t newLockId) noexcept {
        truckId_ = std::move(newTruckId);
        lockId_ = newLockId;
        active_ = true;
    }

    void invalidate() noexcept { active_ = false; }

private:
    AgentId truckId_;
    ContractId contractId_;
    uint64_t lockId_{0};
    bool active_{false};
};

/**
 * @brief Move-only handle for held farmer escrow deposit.
 */
class EscrowHandle {
public:
    EscrowHandle() = default;

    EscrowHandle(AgentId farmerId, ContractId contractId, Money amount, uint64_t escrowId)
        : farmerId_(std::move(farmerId)),
          contractId_(contractId),
          amount_(amount),
          escrowId_(escrowId),
          active_(true) {}

    ~EscrowHandle() = default;

    EscrowHandle(const EscrowHandle&) = delete;
    EscrowHandle& operator=(const EscrowHandle&) = delete;

    EscrowHandle(EscrowHandle&& other) noexcept
        : farmerId_(std::move(other.farmerId_)),
          contractId_(other.contractId_),
          amount_(other.amount_),
          escrowId_(other.escrowId_),
          active_(other.active_) {
        other.active_ = false;
    }

    EscrowHandle& operator=(EscrowHandle&& other) noexcept {
        if (this != &other) {
            farmerId_ = std::move(other.farmerId_);
            contractId_ = other.contractId_;
            amount_ = other.amount_;
            escrowId_ = other.escrowId_;
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return active_; }
    [[nodiscard]] const AgentId& farmerId() const noexcept { return farmerId_; }
    [[nodiscard]] ContractId contractId() const noexcept { return contractId_; }
    [[nodiscard]] Money amount() const noexcept { return amount_; }
    [[nodiscard]] uint64_t escrowId() const noexcept { return escrowId_; }

    void invalidate() noexcept { active_ = false; }

private:
    AgentId farmerId_;
    ContractId contractId_;
    Money amount_{0};
    uint64_t escrowId_{0};
    bool active_{false};
};

/**
 * @brief Port for reserving and releasing mandi gate slots.
 * Sole authority is the APMCMandiAgent.
 */
class ISlotPort {
public:
    virtual ~ISlotPort() = default;

    [[nodiscard]] virtual std::optional<SlotReservationHandle> reserveSlot(
        const AgentId& mandiId,
        const ContractId& contractId,
        SimTime slotTime) = 0;

    virtual bool cancelSlot(SlotReservationHandle& handle) = 0;
};

/**
 * @brief Port for locking and releasing trucks during contract execution.
 */
class ITruckLockPort {
public:
    virtual ~ITruckLockPort() = default;

    [[nodiscard]] virtual std::optional<TruckLockHandle> lockTruck(
        const AgentId& truckId,
        const ContractId& contractId) = 0;

    virtual bool releaseTruck(TruckLockHandle& handle) = 0;

    virtual bool rebindTruck(
        TruckLockHandle& handle,
        const AgentId& newTruckId) = 0;
};

/**
 * @brief Port for holding, refunding, and transferring escrow deposits.
 */
class IEscrowPort {
public:
    virtual ~IEscrowPort() = default;

    [[nodiscard]] virtual std::optional<EscrowHandle> holdDeposit(
        const AgentId& farmerId,
        const ContractId& contractId,
        Money amount) = 0;

    virtual bool refundDeposit(EscrowHandle& handle) = 0;

    virtual bool transferDeposit(
        EscrowHandle& handle,
        const AgentId& recipientId,
        Money transferAmount) = 0;
};

} // namespace kisansetu::contracts
