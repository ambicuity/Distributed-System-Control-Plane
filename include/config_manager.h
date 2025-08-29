#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <nlohmann/json.hpp>

namespace dcp {

class ConfigManager {
private:
    std::unordered_map<std::string, nlohmann::json> config_;
    std::string configFile_;
    mutable std::mutex mutex_;
    
public:
    ConfigManager(const std::string& configFile = "config.json");
    
    bool loadConfig();
    bool saveConfig();
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;
    
    template<typename T>
    void set(const std::string& key, const T& value);
    
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    
    nlohmann::json getSection(const std::string& section) const;
    void setSection(const std::string& section, const nlohmann::json& value);
    
    std::string toString() const;
};

// Template implementations
template<typename T>
T ConfigManager::get(const std::string& key, const T& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_.find(key);
    if (it != config_.end() && !it->second.is_null()) {
        try {
            return it->second.get<T>();
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    return defaultValue;
}

template<typename T>
void ConfigManager::set(const std::string& key, const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_[key] = value;
}

} // namespace dcp