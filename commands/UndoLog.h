#pragma once

#include <vector>
#include <functional>
#include <string>

namespace agro {

enum class UndoActionType {
    ReleaseTruck,
    CancelSlot,
    RefundEscrow
};

struct UndoStep {
    UndoActionType type;
    std::string description;
    std::function<bool()> compensateAction;
};

class UndoLog {
public:
    void record(UndoActionType type, std::string description, std::function<bool()> action) {
        m_steps.push_back(UndoStep{type, std::move(description), std::move(action)});
    }

    [[nodiscard]] size_t size() const noexcept {
        return m_steps.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_steps.empty();
    }

    // Executes all compensation steps in strictly reverse order of acquisition
    bool rollbackAll() {
        bool allSucceeded = true;
        while (!m_steps.empty()) {
            auto step = std::move(m_steps.back());
            m_steps.pop_back();
            if (step.compensateAction) {
                bool ok = step.compensateAction();
                if (!ok) {
                    allSucceeded = false;
                }
            }
        }
        return allSucceeded;
    }

    // Rollback only a specific action type (e.g. for rebind)
    bool rollbackType(UndoActionType targetType) {
        for (auto it = m_steps.rbegin(); it != m_steps.rend(); ++it) {
            if (it->type == targetType) {
                bool ok = it->compensateAction ? it->compensateAction() : true;
                // Remove the element
                auto forwardIt = (it + 1).base();
                m_steps.erase(forwardIt);
                return ok;
            }
        }
        return false;
    }

    void clear() noexcept {
        m_steps.clear();
    }

private:
    std::vector<UndoStep> m_steps;
};

} // namespace agro
