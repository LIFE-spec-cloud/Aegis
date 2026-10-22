#pragma once

#include <string>

namespace agro {

enum class CommandStatus {
    Success,
    Failed,
    AlreadyExecuted,
    AlreadyCompensated
};

class ICommand {
public:
    virtual ~ICommand() = default;

    [[nodiscard]] virtual CommandStatus execute() = 0;
    [[nodiscard]] virtual CommandStatus compensate() = 0;
    [[nodiscard]] virtual CommandStatus abort() = 0;
};

} // namespace agro
