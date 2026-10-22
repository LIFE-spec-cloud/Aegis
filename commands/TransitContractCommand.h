#pragma once

#include "ICommand.h"
#include "ReservationGuards.h"
#include "UndoLog.h"
#include "../include/contracts/Types.h"
#include "../include/contracts/IReservationPorts.h"
#include "../include/contracts/EventLog.h"

#include <string>
#include <optional>
#include <memory>

namespace agro {


struct TransitContractParams {
    ContractId contractId;
    AgentId farmerId;
    AgentId transporterId;
    AgentId mandiId;
    SimTime slotTime{0};
    Money depositAmount{0};
    Money freightCost{0};
    SimTime createdAt{0};
};

class TransitContractCommand : public ICommand {
public:
    TransitContractCommand(
        TransitContractParams params,
        ITruckLockPort& truckPort,
        ISlotPort& slotPort,
        IEscrowPort& escrowPort);

    ~TransitContractCommand() override;

    TransitContractCommand(const TransitContractCommand&) = delete;
    TransitContractCommand& operator=(const TransitContractCommand&) = delete;

    TransitContractCommand(TransitContractCommand&&) noexcept;
    TransitContractCommand& operator=(TransitContractCommand&&) noexcept;

    // 2-Phase Commit execution: acquires Truck Lock -> Gate Slot -> Escrow
    [[nodiscard]] CommandStatus execute() override;

    // Reverse-order idempotent compensation: Escrow -> Gate Slot -> Truck Lock
    [[nodiscard]] CommandStatus compensate() override;

    // Abort during PREPARING or before COMMIT
    [[nodiscard]] CommandStatus abort() override;

    // Rebind slot for Mandi glut / dynamic rerouting
    [[nodiscard]] bool rebindSlot(const AgentId& newMandiId, SimTime newSlotTime, SimTime currentTime);

    // Rebind truck for breakdown / peer handoff
    [[nodiscard]] bool rebindTruck(const AgentId& newTruckId, SimTime currentTime);

    // Divert to processor: releases mandi gate slot, rebinds destination to processing plant
    [[nodiscard]] bool divertToProcessor(const AgentId& processorId, Money processingPrice, SimTime currentTime);

    // Finalize successful contract upon delivery
    [[nodiscard]] bool complete(SimTime currentTime);

    // State queries
    [[nodiscard]] ContractState state() const noexcept { return m_state; }
    [[nodiscard]] uint64_t version() const noexcept { return m_version; }
    [[nodiscard]] const ContractId& contractId() const noexcept { return m_params.contractId; }
    [[nodiscard]] const AgentId& farmerId() const noexcept { return m_params.farmerId; }
    [[nodiscard]] const AgentId& transporterId() const noexcept { return m_params.transporterId; }
    [[nodiscard]] const AgentId& mandiId() const noexcept { return m_params.mandiId; }
    [[nodiscard]] SimTime slotTime() const noexcept { return m_params.slotTime; }
    [[nodiscard]] Money depositAmount() const noexcept { return m_params.depositAmount; }

    [[nodiscard]] bool isTerminal() const noexcept {
        return m_state == ContractState::COMPLETED ||
               m_state == ContractState::COMPENSATED ||
               m_state == ContractState::ABORTED;
    }

    [[nodiscard]] bool hasTruckLock() const noexcept { return m_truckLock.has_value() && m_truckLock->isValid(); }
    [[nodiscard]] bool hasSlot() const noexcept { return m_slotReservation.has_value() && m_slotReservation->isValid(); }
    [[nodiscard]] bool hasEscrow() const noexcept { return m_escrow.has_value() && m_escrow->isValid(); }

private:
    void logStateTransition(ContractState from, ContractState to, SimTime time, const std::string& note);

    TransitContractParams m_params;
    ITruckLockPort& m_truckPort;
    ISlotPort& m_slotPort;
    IEscrowPort& m_escrowPort;

    ContractState m_state{ContractState::CREATED};
    uint64_t m_version{1};

    std::optional<TruckLockHandle> m_truckLock;
    std::optional<SlotReservationHandle> m_slotReservation;
    std::optional<EscrowHandle> m_escrow;

    UndoLog m_undoLog;
};

} // namespace agro
