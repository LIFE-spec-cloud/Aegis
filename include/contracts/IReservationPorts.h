#pragma once

#include "Types.h"
#include <optional>
#include <utility>
#include <string>

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
          reservationStr_("slot_res_" + std::to_string(reservationId)),
          active_(true) {}

    SlotReservationHandle(std::string resStr, AgentId mandiId, SimTime slotTime, ContractId contractId)
        : mandiId_(std::move(mandiId)),
          contractId_(contractId),
          slotTime_(slotTime),
          reservationId_(1),
          reservationStr_(std::move(resStr)),
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
          reservationStr_(std::move(other.reservationStr_)),
          active_(other.active_) {
        other.active_ = false;
    }

    SlotReservationHandle& operator=(SlotReservationHandle&& other) noexcept {
        if (this != &other) {
            mandiId_ = std::move(other.mandiId_);
            contractId_ = other.contractId_;
            slotTime_ = other.slotTime_;
            reservationId_ = other.reservationId_;
            reservationStr_ = std::move(other.reservationStr_);
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
    [[nodiscard]] const std::string& reservationIdStr() const noexcept { return reservationStr_; }

    void invalidate() noexcept { active_ = false; }

private:
    AgentId mandiId_;
    ContractId contractId_{0};
    SimTime slotTime_{0};
    uint64_t reservationId_{0};
    std::string reservationStr_;
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
          lockStr_("truck_lock_" + std::to_string(lockId)),
          active_(true) {}

    TruckLockHandle(std::string lockStr, AgentId truckId, ContractId contractId)
        : truckId_(std::move(truckId)),
          contractId_(contractId),
          lockId_(1),
          lockStr_(std::move(lockStr)),
          active_(true) {}

    ~TruckLockHandle() = default;

    TruckLockHandle(const TruckLockHandle&) = delete;
    TruckLockHandle& operator=(const TruckLockHandle&) = delete;

    TruckLockHandle(TruckLockHandle&& other) noexcept
        : truckId_(std::move(other.truckId_)),
          contractId_(other.contractId_),
          lockId_(other.lockId_),
          lockStr_(std::move(other.lockStr_)),
          active_(other.active_) {
        other.active_ = false;
    }

    TruckLockHandle& operator=(TruckLockHandle&& other) noexcept {
        if (this != &other) {
            truckId_ = std::move(other.truckId_);
            contractId_ = other.contractId_;
            lockId_ = other.lockId_;
            lockStr_ = std::move(other.lockStr_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return active_; }
    [[nodiscard]] const AgentId& truckId() const noexcept { return truckId_; }
    [[nodiscard]] ContractId contractId() const noexcept { return contractId_; }
    [[nodiscard]] uint64_t lockId() const noexcept { return lockId_; }
    [[nodiscard]] const std::string& lockIdStr() const noexcept { return lockStr_; }

    void rebind(AgentId newTruckId, uint64_t newLockId = 0) noexcept {
        truckId_ = std::move(newTruckId);
        if (newLockId > 0) {
            lockId_ = newLockId;
            lockStr_ = "truck_lock_" + std::to_string(newLockId);
        }
        active_ = true;
    }

    void setTruckId(AgentId newTruckId) { rebind(std::move(newTruckId)); }
    void invalidate() noexcept { active_ = false; }

private:
    AgentId truckId_;
    ContractId contractId_{0};
    uint64_t lockId_{0};
    std::string lockStr_;
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
          escrowStr_("escrow_" + std::to_string(escrowId)),
          active_(true) {}

    EscrowHandle(std::string escrowStr, AgentId payerId, Money amount, ContractId contractId)
        : farmerId_(std::move(payerId)),
          contractId_(contractId),
          amount_(amount),
          escrowId_(1),
          escrowStr_(std::move(escrowStr)),
          active_(true) {}

    ~EscrowHandle() = default;

    EscrowHandle(const EscrowHandle&) = delete;
    EscrowHandle& operator=(const EscrowHandle&) = delete;

    EscrowHandle(EscrowHandle&& other) noexcept
        : farmerId_(std::move(other.farmerId_)),
          contractId_(other.contractId_),
          amount_(other.amount_),
          escrowId_(other.escrowId_),
          escrowStr_(std::move(other.escrowStr_)),
          active_(other.active_) {
        other.active_ = false;
    }

    EscrowHandle& operator=(EscrowHandle&& other) noexcept {
        if (this != &other) {
            farmerId_ = std::move(other.farmerId_);
            contractId_ = other.contractId_;
            amount_ = other.amount_;
            escrowId_ = other.escrowId_;
            escrowStr_ = std::move(other.escrowStr_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    [[nodiscard]] bool isValid() const noexcept { return active_; }
    [[nodiscard]] const AgentId& farmerId() const noexcept { return farmerId_; }
    [[nodiscard]] const AgentId& payerId() const noexcept { return farmerId_; }
    [[nodiscard]] ContractId contractId() const noexcept { return contractId_; }
    [[nodiscard]] Money amount() const noexcept { return amount_; }
    [[nodiscard]] uint64_t escrowId() const noexcept { return escrowId_; }
    [[nodiscard]] const std::string& escrowIdStr() const noexcept { return escrowStr_; }

    void setAmount(Money newAmount) noexcept { amount_ = newAmount; }
    void invalidate() noexcept { active_ = false; }

private:
    AgentId farmerId_;
    ContractId contractId_{0};
    Money amount_{0};
    uint64_t escrowId_{0};
    std::string escrowStr_;
    bool active_{false};
};

/**
 * @brief Port for reserving and releasing mandi gate slots.
 */
class ISlotPort {
public:
    virtual ~ISlotPort() = default;

    [[nodiscard]] virtual std::optional<SlotReservationHandle> reserveSlot(
        const AgentId& mandiId,
        const ContractId& contractId,
        SimTime slotTime) {
        return reserveSlot(mandiId, slotTime, contractId);
    }

    [[nodiscard]] virtual std::optional<SlotReservationHandle> reserveSlot(
        const AgentId& mandiId,
        SimTime slotTime,
        const ContractId& contractId) {
        return reserveSlot(mandiId, contractId, slotTime);
    }

    virtual bool cancelSlot(SlotReservationHandle& handle) {
        return cancelSlot(static_cast<const SlotReservationHandle&>(handle));
    }

    virtual bool cancelSlot(const SlotReservationHandle& handle) {
        (void)handle;
        return false;
    }
};

/**
 * @brief Port for locking and releasing trucks during contract execution.
 */
class ITruckLockPort {
public:
    virtual ~ITruckLockPort() = default;

    [[nodiscard]] virtual std::optional<TruckLockHandle> lockTruck(
        const AgentId& truckId,
        const ContractId& contractId) {
        (void)truckId; (void)contractId;
        return std::nullopt;
    }

    virtual bool releaseTruck(TruckLockHandle& handle) {
        return releaseTruck(static_cast<const TruckLockHandle&>(handle));
    }

    virtual bool releaseTruck(const TruckLockHandle& handle) {
        (void)handle;
        return false;
    }

    virtual bool rebindTruck(
        TruckLockHandle& handle,
        const AgentId& newTruckId) {
        (void)handle; (void)newTruckId;
        return false;
    }
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
        Money amount) {
        return holdDeposit(farmerId, amount, contractId);
    }

    [[nodiscard]] virtual std::optional<EscrowHandle> holdDeposit(
        const AgentId& payerId,
        Money amount,
        const ContractId& contractId) {
        return holdDeposit(payerId, contractId, amount);
    }

    virtual bool refundDeposit(EscrowHandle& handle) {
        return refundEscrow(handle);
    }

    virtual bool refundEscrow(
        const EscrowHandle& handle,
        Money refundAmount = Money(0)) {
        (void)handle; (void)refundAmount;
        return false;
    }

    virtual bool transferDeposit(
        EscrowHandle& handle,
        const AgentId& recipientId,
        Money transferAmount) {
        return transferEscrow(handle, recipientId, transferAmount);
    }

    virtual bool transferEscrow(
        const EscrowHandle& handle,
        const AgentId& toAgent,
        Money amount) {
        (void)handle; (void)toAgent; (void)amount;
        return false;
    }
};

} // namespace kisansetu::contracts
