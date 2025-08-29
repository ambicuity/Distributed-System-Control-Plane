#include "config_manager.h"
#include <fstream>
#include <iostream>

namespace dcp {

ConfigManager::ConfigManager(const std::string& configFile) : configFile_(configFile) {
    loadConfig();
}

bool ConfigManager::loadConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(configFile_);
    if (!file.is_open()) {
        // Create default configuration
        config_["server"] = nlohmann::json::object();
        config_["server"]["port"] = 8080;
        config_["server"]["host"] = "0.0.0.0";
        
        config_["health_check"] = nlohmann::json::object();
        config_["health_check"]["interval_ms"] = 30000;
        config_["health_check"]["timeout_ms"] = 5000;
        
        config_["load_balancer"] = nlohmann::json::object();
        config_["load_balancer"]["algorithm"] = "round_robin";
        
        config_["monitoring"] = nlohmann::json::object();
        config_["monitoring"]["enabled"] = true;
        config_["monitoring"]["export_interval_ms"] = 10000;
        
        return saveConfig();
    }
    
    try {
        nlohmann::json jsonData;
        file >> jsonData;
        
        config_.clear();
        for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
            config_[it.key()] = it.value();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::saveConfig() {
    // Skip file saving for now - in-memory config only for this demo
    return true;
}

bool ConfigManager::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.find(key) != config_.end();
}

void ConfigManager::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.erase(key);
}

nlohmann::json ConfigManager::getSection(const std::string& section) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_.find(section);
    if (it != config_.end()) {
        return it->second;
    }
    return nlohmann::json::object();
}

void ConfigManager::setSection(const std::string& section, const nlohmann::json& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_[section] = value;
}

std::string ConfigManager::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json jsonData;
    for (const auto& [key, value] : config_) {
        jsonData[key] = value;
    }
    return jsonData.dump(4);
}

} // namespace dcp