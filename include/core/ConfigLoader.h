#pragma once

#include "contracts/Types.h"
#include <string>
#include <unordered_map>
#include <optional>

namespace kisansetu::core {

/**
 * @brief Thread-safe configuration reader loading parameters from config/default.cfg.
 */
class ConfigLoader {
public:
    ConfigLoader() = default;

    /**
     * @brief Load configuration from a file on disk.
     */
    bool load(const std::string& filepath);

    /**
     * @brief Load configuration from an in-memory string.
     */
    bool loadFromString(const std::string& content);

    [[nodiscard]] std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    [[nodiscard]] double getDouble(const std::string& key, double defaultValue = 0.0) const;
    [[nodiscard]] int64_t getInt(const std::string& key, int64_t defaultValue = 0) const;
    [[nodiscard]] bool getBool(const std::string& key, bool defaultValue = false) const;
    [[nodiscard]] contracts::Money getMoney(const std::string& key, contracts::Money defaultValue = contracts::Money(0)) const;

    [[nodiscard]] bool hasKey(const std::string& key) const;
    [[nodiscard]] size_t size() const;
    void clear();

private:
    std::unordered_map<std::string, std::string> entries_;
};

} // namespace kisansetu::core
