#pragma once

#include "TransitContractCommand.h"
#include "../tests/fakes/FakeTruckLockPort.h"
#include "../tests/fakes/FakeSlotPort.h"
#include "../tests/fakes/FakeEscrowPort.h"

#include <vector>
#include <string>
#include <sstream>

namespace agro {

struct InvariantCheckResult {
    bool passed{true};
    std::string diagnosticMessage;
};

class InvariantChecker {
public:
    static InvariantCheckResult verify(
        const std::vector<const TransitContractCommand*>& contracts,
        const fakes::FakeTruckLockPort& truckPort,
        const fakes::FakeSlotPort& slotPort,
        const fakes::FakeEscrowPort& escrowPort) {
        
        size_t expectedTruckLocks = 0;
        size_t expectedSlots = 0;
        size_t expectedEscrows = 0;

        for (const auto* contract : contracts) {
            if (!contract) continue;

            if (contract->isTerminal()) {
                // Invariant 1: Terminal contracts must never own active resource handles
                if (contract->hasTruckLock() || contract->hasSlot() || contract->hasEscrow()) {
                    std::ostringstream oss;
                    oss << "Invariant Violation: Terminal contract " << contract->contractId().value
                        << " still holds resources! (truckLock=" << contract->hasTruckLock()
                        << ", slot=" << contract->hasSlot()
                        << ", escrow=" << contract->hasEscrow() << ")";
                    return {false, oss.str()};
                }
            } else {
                // Non-terminal contracts
                if (contract->hasTruckLock()) expectedTruckLocks++;
                if (contract->hasSlot()) expectedSlots++;
                if (contract->hasEscrow()) expectedEscrows++;
            }
        }

        // Invariant 2: Active resources in ports must exactly equal resources owned by non-terminal contracts
        size_t actualTruckLocks = truckPort.activeLockCount();
        size_t actualSlots = slotPort.activeReservationCount();
        size_t actualEscrows = escrowPort.activeEscrowCount();

        if (actualTruckLocks != expectedTruckLocks ||
            actualSlots != expectedSlots ||
            actualEscrows != expectedEscrows) {
            
            std::ostringstream oss;
            oss << "Invariant Violation: Resource leak or discrepancy detected!\n"
                << "  Truck locks: port has " << actualTruckLocks << ", contracts hold " << expectedTruckLocks << "\n"
                << "  Gate slots:  port has " << actualSlots << ", contracts hold " << expectedSlots << "\n"
                << "  Escrow holds: port has " << actualEscrows << ", contracts hold " << expectedEscrows;
            return {false, oss.str()};
        }

        return {true, "All resource invariants verified: zero leaked resources."};
    }
};

} // namespace agro
