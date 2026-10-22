#include "TransitContractCommand.h"

namespace agro {

TransitContractCommand::TransitContractCommand(
    TransitContractParams params,
    ITruckLockPort& truckPort,
    ISlotPort& slotPort,
    IEscrowPort& escrowPort)
    : m_params(std::move(params)),
      m_truckPort(truckPort),
      m_slotPort(slotPort),
      m_escrowPort(escrowPort),
      m_state(ContractState::CREATED),
      m_version(1) {
    
    logStateTransition(ContractState::CREATED, ContractState::CREATED, m_params.createdAt, "Contract instance created");
}

TransitContractCommand::~TransitContractCommand() {
    if (m_state == ContractState::PREPARING) {
        static_cast<void>(abort());
    }
}

TransitContractCommand::TransitContractCommand(TransitContractCommand&& other) noexcept
    : m_params(std::move(other.m_params)),
      m_truckPort(other.m_truckPort),
      m_slotPort(other.m_slotPort),
      m_escrowPort(other.m_escrowPort),
      m_state(other.m_state),
      m_version(other.m_version),
      m_truckLock(std::move(other.m_truckLock)),
      m_slotReservation(std::move(other.m_slotReservation)),
      m_escrow(std::move(other.m_escrow)),
      m_undoLog(std::move(other.m_undoLog)) {
    other.m_state = ContractState::ABORTED;
}

TransitContractCommand& TransitContractCommand::operator=(TransitContractCommand&& other) noexcept {
    if (this != &other) {
        if (m_state == ContractState::PREPARING || m_state == ContractState::COMMITTED) {
            static_cast<void>(compensate());
        }
        m_params = std::move(other.m_params);
        m_state = other.m_state;
        m_version = other.m_version;
        m_truckLock = std::move(other.m_truckLock);
        m_slotReservation = std::move(other.m_slotReservation);
        m_escrow = std::move(other.m_escrow);
        m_undoLog = std::move(other.m_undoLog);
        other.m_state = ContractState::ABORTED;
    }
    return *this;
}

CommandStatus TransitContractCommand::execute() {
    if (m_state != ContractState::CREATED) {
        return CommandStatus::AlreadyExecuted;
    }

    logStateTransition(m_state, ContractState::PREPARING, m_params.createdAt, "Starting 2PC Prepare phase");
    m_state = ContractState::PREPARING;

    // STEP 1: Lock Truck
    auto truckRes = m_truckPort.lockTruck(m_params.transporterId, m_params.contractId);
    if (!truckRes.has_value()) {
        EventLog::instance().append(
            m_params.createdAt, EventLevel::WARNING, "COMMAND",
            "Prepare Step 1 failed: unable to lock truck " + m_params.transporterId.value);
        abort();
        return CommandStatus::Failed;
    }
    m_truckLock = std::move(*truckRes);
    m_undoLog.record(
        UndoActionType::ReleaseTruck,
        "Release truck lock " + m_params.transporterId.value,
        [this]() {
            if (m_truckLock && m_truckLock->isValid()) {
                bool ok = m_truckPort.releaseTruck(*m_truckLock);
                m_truckLock->invalidate();
                return ok;
            }
            return true;
        });

    // STEP 2: Reserve Mandi Gate Slot
    auto slotRes = m_slotPort.reserveSlot(m_params.mandiId, m_params.slotTime, m_params.contractId);
    if (!slotRes.has_value()) {
        EventLog::instance().append(
            m_params.createdAt, EventLevel::WARNING, "COMMAND",
            "Prepare Step 2 failed: unable to reserve slot at " + m_params.mandiId.value);
        abort();
        return CommandStatus::Failed;
    }
    m_slotReservation = std::move(*slotRes);
    m_undoLog.record(
        UndoActionType::CancelSlot,
        "Cancel gate slot at " + m_params.mandiId.value,
        [this]() {
            if (m_slotReservation && m_slotReservation->isValid()) {
                bool ok = m_slotPort.cancelSlot(*m_slotReservation);
                m_slotReservation->invalidate();
                return ok;
            }
            return true;
        });

    // STEP 3: Hold Farmer Escrow Deposit
    auto escrowRes = m_escrowPort.holdDeposit(m_params.farmerId, m_params.depositAmount, m_params.contractId);
    if (!escrowRes.has_value()) {
        EventLog::instance().append(
            m_params.createdAt, EventLevel::WARNING, "COMMAND",
            "Prepare Step 3 failed: unable to hold escrow deposit from " + m_params.farmerId.value);
        abort();
        return CommandStatus::Failed;
    }
    m_escrow = std::move(*escrowRes);
    m_undoLog.record(
        UndoActionType::RefundEscrow,
        "Refund escrow deposit to " + m_params.farmerId.value,
        [this]() {
            if (m_escrow && m_escrow->isValid()) {
                bool ok = m_escrowPort.refundEscrow(*m_escrow, m_escrow->amount());
                m_escrow->invalidate();
                return ok;
            }
            return true;
        });

    // All resources prepared successfully -> COMMIT
    logStateTransition(m_state, ContractState::COMMITTED, m_params.createdAt, "2PC Commit succeeded - Contract is ACTIVE");
    m_state = ContractState::COMMITTED;
    ++m_version;

    return CommandStatus::Success;
}

CommandStatus TransitContractCommand::compensate() {
    if (m_state == ContractState::COMPENSATED) {
        // Idempotent: calling twice changes nothing
        return CommandStatus::Success;
    }

    if (m_state == ContractState::ABORTED) {
        return CommandStatus::Success;
    }

    if (m_state == ContractState::CREATED) {
        m_state = ContractState::ABORTED;
        return CommandStatus::Success;
    }

    logStateTransition(
        m_state, ContractState::COMPENSATED, m_params.createdAt,
        "Compensating contract resources in reverse order (v" + std::to_string(m_version) + ")");

    // Execute reverse rollback: Escrow -> Slot -> Truck
    bool rollbackOk = m_undoLog.rollbackAll();
    m_state = ContractState::COMPENSATED;
    ++m_version;

    if (m_escrow) m_escrow->invalidate();
    if (m_slotReservation) m_slotReservation->invalidate();
    if (m_truckLock) m_truckLock->invalidate();

    return rollbackOk ? CommandStatus::Success : CommandStatus::Failed;
}

CommandStatus TransitContractCommand::abort() {
    if (m_state == ContractState::ABORTED || m_state == ContractState::COMPENSATED) {
        return CommandStatus::Success;
    }

    logStateTransition(m_state, ContractState::ABORTED, m_params.createdAt, "Aborting contract prepare");

    m_undoLog.rollbackAll();
    m_state = ContractState::ABORTED;
    ++m_version;

    if (m_escrow) m_escrow->invalidate();
    if (m_slotReservation) m_slotReservation->invalidate();
    if (m_truckLock) m_truckLock->invalidate();

    return CommandStatus::Success;
}

bool TransitContractCommand::rebindSlot(const AgentId& newMandiId, SimTime newSlotTime, SimTime currentTime) {
    if (m_state != ContractState::COMMITTED) {
        return false;
    }

    EventLog::instance().append(
        currentTime, EventLevel::ROLLBACK, "COMMAND",
        "Rebinding slot: canceling old slot at " + m_params.mandiId.value + 
        " for new slot at " + newMandiId.value);

    // Cancel old slot
    if (m_slotReservation && m_slotReservation->isValid()) {
        m_slotPort.cancelSlot(*m_slotReservation);
        m_slotReservation->invalidate();
    }
    m_undoLog.rollbackType(UndoActionType::CancelSlot);

    // Reserve new slot
    auto newSlot = m_slotPort.reserveSlot(newMandiId, newSlotTime, m_params.contractId);
    if (!newSlot.has_value()) {
        EventLog::instance().append(
            currentTime, EventLevel::FAULT, "COMMAND",
            "Failed to reserve replacement slot at " + newMandiId.value);
        return false;
    }

    m_slotReservation = std::move(*newSlot);
    m_params.mandiId = newMandiId;
    m_params.slotTime = newSlotTime;

    m_undoLog.record(
        UndoActionType::CancelSlot,
        "Cancel rebound gate slot at " + newMandiId.value,
        [this]() {
            if (m_slotReservation && m_slotReservation->isValid()) {
                bool ok = m_slotPort.cancelSlot(*m_slotReservation);
                m_slotReservation->invalidate();
                return ok;
            }
            return true;
        });

    ++m_version;
    EventLog::instance().append(
        currentTime, EventLevel::RECOVERY, "COMMAND",
        "Slot successfully rebound to " + newMandiId.value + " (contract v" + std::to_string(m_version) + ")");

    return true;
}

bool TransitContractCommand::rebindTruck(const AgentId& newTruckId, SimTime currentTime) {
    if (m_state != ContractState::COMMITTED || !m_truckLock || !m_truckLock->isValid()) {
        return false;
    }

    EventLog::instance().append(
        currentTime, EventLevel::ROLLBACK, "COMMAND",
        "Rebinding truck: transferring manifest from " + m_params.transporterId.value + 
        " to " + newTruckId.value);

    bool rebound = m_truckPort.rebindTruck(*m_truckLock, newTruckId);
    if (!rebound) {
        EventLog::instance().append(
            currentTime, EventLevel::FAULT, "COMMAND",
            "Failed to rebind truck to peer " + newTruckId.value);
        return false;
    }

    m_params.transporterId = newTruckId;
    ++m_version;

    EventLog::instance().append(
        currentTime, EventLevel::RECOVERY, "COMMAND",
        "Truck successfully rebound to peer " + newTruckId.value + " (contract v" + std::to_string(m_version) + ")");

    return true;
}

bool TransitContractCommand::divertToProcessor(
    const AgentId& processorId,
    Money /*processingPrice*/,
    SimTime currentTime) {
    
    if (m_state != ContractState::COMMITTED) {
        return false;
    }

    EventLog::instance().append(
        currentTime, EventLevel::ROLLBACK, "COMMAND",
        "Diverting to processing plant " + processorId.value + 
        ": releasing mandi gate slot at " + m_params.mandiId.value);

    // Release mandi gate slot
    if (m_slotReservation && m_slotReservation->isValid()) {
        m_slotPort.cancelSlot(*m_slotReservation);
        m_slotReservation->invalidate();
    }
    m_undoLog.rollbackType(UndoActionType::CancelSlot);

    m_params.mandiId = processorId;
    ++m_version;

    EventLog::instance().append(
        currentTime, EventLevel::RECOVERY, "COMMAND",
        "Contract " + m_params.contractId.value + " successfully diverted to processor " + 
        processorId.value + " (zero slot locks held, contract v" + std::to_string(m_version) + ")");

    return true;
}

bool TransitContractCommand::complete(SimTime currentTime) {
    if (m_state != ContractState::COMMITTED) {
        return false;
    }

    logStateTransition(m_state, ContractState::COMPLETED, currentTime, "Delivery fulfilled successfully");

    // Settle escrow: transfer agreed freight to transporter, refund remaining to farmer
    if (m_escrow && m_escrow->isValid()) {
        Money freight = m_params.freightCost;
        if (freight.paise > 0 && freight.paise <= m_escrow->amount().paise) {
            m_escrowPort.transferEscrow(*m_escrow, m_params.transporterId, freight);
            m_escrow->setAmount(m_escrow->amount() - freight);
        }
        Money remaining = m_escrow->amount();
        if (remaining.paise > 0) {
            m_escrowPort.refundEscrow(*m_escrow, remaining);
            m_escrow->setAmount(Money(0));
        }
        m_escrow->invalidate();
    }

    // Release truck lock
    if (m_truckLock && m_truckLock->isValid()) {
        m_truckPort.releaseTruck(*m_truckLock);
        m_truckLock->invalidate();
    }

    // Mark slot completed / freed
    if (m_slotReservation && m_slotReservation->isValid()) {
        m_slotPort.cancelSlot(*m_slotReservation);
        m_slotReservation->invalidate();
    }

    m_undoLog.clear();
    m_state = ContractState::COMPLETED;
    ++m_version;

    return true;
}

void TransitContractCommand::logStateTransition(
    ContractState from,
    ContractState to,
    SimTime time,
    const std::string& note) {
    
    auto stateToStr = [](ContractState s) -> std::string {
        switch (s) {
            case ContractState::CREATED: return "CREATED";
            case ContractState::PREPARING: return "PREPARING";
            case ContractState::COMMITTED: return "COMMITTED";
            case ContractState::COMPLETED: return "COMPLETED";
            case ContractState::COMPENSATED: return "COMPENSATED";
            case ContractState::ABORTED: return "ABORTED";
        }
        return "UNKNOWN";
    };

    EventLevel lvl = EventLevel::INFO;
    if (to == ContractState::COMPENSATED || to == ContractState::ABORTED) {
        lvl = EventLevel::ROLLBACK;
    } else if (to == ContractState::COMPLETED) {
        lvl = EventLevel::RECOVERY;
    }

    std::string msg = "Contract " + m_params.contractId.value + " [" + 
                      stateToStr(from) + " -> " + stateToStr(to) + "] (v" + 
                      std::to_string(m_version) + "): " + note;
    EventLog::instance().append(time, lvl, "CONTRACT_STATE", msg);
}

} // namespace agro
