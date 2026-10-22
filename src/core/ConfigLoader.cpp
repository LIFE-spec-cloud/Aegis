#include "core/ConfigLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace kisansetu::core {

namespace {

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    if (start == str.size()) {
        return "";
    }
    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        --end;
    }
    return str.substr(start, end - start + 1);
}

} // namespace

bool ConfigLoader::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        // Fallback for running from build/ subdirectory
        std::ifstream fallback("../" + filepath);
        if (fallback.is_open()) {
            std::stringstream buffer;
            buffer << fallback.rdbuf();
            return loadFromString(buffer.str());
        }
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

bool ConfigLoader::loadFromString(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;

    while (std::getline(stream, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        // Section header [section]
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            currentSection = trim(trimmed.substr(1, trimmed.size() - 2));
            continue;
        }

        // Key = Value
        size_t eqPos = trimmed.find('=');
        if (eqPos != std::string::npos) {
            std::string key = trim(trimmed.substr(0, eqPos));
            std::string val = trim(trimmed.substr(eqPos + 1));

            std::string fullKey = currentSection.empty() ? key : (currentSection + "." + key);
            entries_[fullKey] = val;
        }
    }

    return true;
}

std::string ConfigLoader::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        return it->second;
    }
    return defaultValue;
}

double ConfigLoader::getDouble(const std::string& key, double defaultValue) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        try {
            return std::stod(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

int64_t ConfigLoader::getInt(const std::string& key, int64_t defaultValue) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        try {
            return std::stoll(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigLoader::getBool(const std::string& key, bool defaultValue) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        std::string lower = it->second;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (lower == "true" || lower == "1" || lower == "yes") {
            return true;
        }
        if (lower == "false" || lower == "0" || lower == "no") {
            return false;
        }
    }
    return defaultValue;
}

contracts::Money ConfigLoader::getMoney(const std::string& key, contracts::Money defaultValue) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        try {
            double rupees = std::stod(it->second);
            return contracts::Money::fromRupees(rupees);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigLoader::hasKey(const std::string& key) const {
    return entries_.find(key) != entries_.end();
}

size_t ConfigLoader::size() const {
    return entries_.size();
}

void ConfigLoader::clear() {
    entries_.clear();
}

} // namespace kisansetu::core
