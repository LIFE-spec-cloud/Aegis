#pragma once

#include "../include/contracts/IReservationPorts.h"
#include <optional>
#include <utility>

namespace agro {

/**
 * Generic RAII Resource Guard.
 * Automatically executes the compensating release action upon scope exit
 * unless explicitly dismissed or released.
 */
template <typename Port, typename Handle, typename Releaser>
class BaseReservationGuard {
public:
    BaseReservationGuard(Port& port, Handle handle)
        : m_port(port), m_handle(std::move(handle)), m_active(true) {}

    ~BaseReservationGuard() {
        if (m_active && m_handle.isValid()) {
            Releaser{}(m_port, m_handle);
            m_handle.invalidate();
        }
    }

    BaseReservationGuard(const BaseReservationGuard&) = delete;
    BaseReservationGuard& operator=(const BaseReservationGuard&) = delete;

    BaseReservationGuard(BaseReservationGuard&& other) noexcept
        : m_port(other.m_port), m_handle(std::move(other.m_handle)), m_active(other.m_active) {
        other.m_active = false;
    }

    BaseReservationGuard& operator=(BaseReservationGuard&& other) noexcept {
        if (this != &other) {
            if (m_active && m_handle.isValid()) {
                Releaser{}(m_port, m_handle);
            }
            m_handle = std::move(other.m_handle);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    [[nodiscard]] bool isActive() const noexcept { return m_active && m_handle.isValid(); }
    [[nodiscard]] const Handle& handle() const noexcept { return m_handle; }
    [[nodiscard]] Handle& handle() noexcept { return m_handle; }

    [[nodiscard]] Handle release() noexcept {
        m_active = false;
        return std::move(m_handle);
    }

    void dismiss() noexcept {
        m_active = false;
    }

private:
    Port& m_port;
    Handle m_handle;
    bool m_active{false};
};

struct TruckReleaser {
    void operator()(ITruckLockPort& port, TruckLockHandle& handle) const {
        port.releaseTruck(handle);
    }
};

struct SlotCanceler {
    void operator()(ISlotPort& port, SlotReservationHandle& handle) const {
        port.cancelSlot(handle);
    }
};

struct EscrowRefunder {
    void operator()(IEscrowPort& port, EscrowHandle& handle) const {
        port.refundEscrow(handle, handle.amount());
    }
};

using TruckReservationGuard = BaseReservationGuard<ITruckLockPort, TruckLockHandle, TruckReleaser>;
using SlotReservationGuard = BaseReservationGuard<ISlotPort, SlotReservationHandle, SlotCanceler>;
using EscrowReservationGuard = BaseReservationGuard<IEscrowPort, EscrowHandle, EscrowRefunder>;

} // namespace agro
