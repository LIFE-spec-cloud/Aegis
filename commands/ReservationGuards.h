#pragma once

#include "../include/contracts/IReservationPorts.h"
#include <optional>
#include <utility>

namespace agro {

class TruckReservationGuard {
public:
    TruckReservationGuard(ITruckLockPort& port, TruckLockHandle handle)
        : m_port(port), m_handle(std::move(handle)), m_active(true) {}

    ~TruckReservationGuard() {
        if (m_active && m_handle.isValid()) {
            m_port.releaseTruck(m_handle);
            m_handle.invalidate();
        }
    }

    TruckReservationGuard(const TruckReservationGuard&) = delete;
    TruckReservationGuard& operator=(const TruckReservationGuard&) = delete;

    TruckReservationGuard(TruckReservationGuard&& other) noexcept
        : m_port(other.m_port), m_handle(std::move(other.m_handle)), m_active(other.m_active) {
        other.m_active = false;
    }

    TruckReservationGuard& operator=(TruckReservationGuard&& other) noexcept {
        if (this != &other) {
            if (m_active && m_handle.isValid()) {
                m_port.releaseTruck(m_handle);
            }
            m_handle = std::move(other.m_handle);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    [[nodiscard]] bool isActive() const noexcept { return m_active && m_handle.isValid(); }
    [[nodiscard]] const TruckLockHandle& handle() const noexcept { return m_handle; }
    [[nodiscard]] TruckLockHandle& handle() noexcept { return m_handle; }

    [[nodiscard]] TruckLockHandle release() noexcept {
        m_active = false;
        return std::move(m_handle);
    }

    void dismiss() noexcept {
        m_active = false;
    }

private:
    ITruckLockPort& m_port;
    TruckLockHandle m_handle;
    bool m_active{false};
};

class SlotReservationGuard {
public:
    SlotReservationGuard(ISlotPort& port, SlotReservationHandle handle)
        : m_port(port), m_handle(std::move(handle)), m_active(true) {}

    ~SlotReservationGuard() {
        if (m_active && m_handle.isValid()) {
            m_port.cancelSlot(m_handle);
            m_handle.invalidate();
        }
    }

    SlotReservationGuard(const SlotReservationGuard&) = delete;
    SlotReservationGuard& operator=(const SlotReservationGuard&) = delete;

    SlotReservationGuard(SlotReservationGuard&& other) noexcept
        : m_port(other.m_port), m_handle(std::move(other.m_handle)), m_active(other.m_active) {
        other.m_active = false;
    }

    SlotReservationGuard& operator=(SlotReservationGuard&& other) noexcept {
        if (this != &other) {
            if (m_active && m_handle.isValid()) {
                m_port.cancelSlot(m_handle);
            }
            m_handle = std::move(other.m_handle);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    [[nodiscard]] bool isActive() const noexcept { return m_active && m_handle.isValid(); }
    [[nodiscard]] const SlotReservationHandle& handle() const noexcept { return m_handle; }
    [[nodiscard]] SlotReservationHandle& handle() noexcept { return m_handle; }

    [[nodiscard]] SlotReservationHandle release() noexcept {
        m_active = false;
        return std::move(m_handle);
    }

    void dismiss() noexcept {
        m_active = false;
    }

private:
    ISlotPort& m_port;
    SlotReservationHandle m_handle;
    bool m_active{false};
};

class EscrowReservationGuard {
public:
    EscrowReservationGuard(IEscrowPort& port, EscrowHandle handle)
        : m_port(port), m_handle(std::move(handle)), m_active(true) {}

    ~EscrowReservationGuard() {
        if (m_active && m_handle.isValid()) {
            m_port.refundEscrow(m_handle, m_handle.amount());
            m_handle.invalidate();
        }
    }

    EscrowReservationGuard(const EscrowReservationGuard&) = delete;
    EscrowReservationGuard& operator=(const EscrowReservationGuard&) = delete;

    EscrowReservationGuard(EscrowReservationGuard&& other) noexcept
        : m_port(other.m_port), m_handle(std::move(other.m_handle)), m_active(other.m_active) {
        other.m_active = false;
    }

    EscrowReservationGuard& operator=(EscrowReservationGuard&& other) noexcept {
        if (this != &other) {
            if (m_active && m_handle.isValid()) {
                m_port.refundEscrow(m_handle, m_handle.amount());
            }
            m_handle = std::move(other.m_handle);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    [[nodiscard]] bool isActive() const noexcept { return m_active && m_handle.isValid(); }
    [[nodiscard]] const EscrowHandle& handle() const noexcept { return m_handle; }
    [[nodiscard]] EscrowHandle& handle() noexcept { return m_handle; }

    [[nodiscard]] EscrowHandle release() noexcept {
        m_active = false;
        return std::move(m_handle);
    }

    void dismiss() noexcept {
        m_active = false;
    }

private:
    IEscrowPort& m_port;
    EscrowHandle m_handle;
    bool m_active{false};
};

} // namespace agro
