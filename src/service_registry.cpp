#include "service_registry.h"
#include <algorithm>
#include <mutex>

namespace dcp {

bool ServiceRegistry::registerService(const std::shared_ptr<Service>& service) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!service) return false;
    
    services_[service->id] = service;
    return true;
}

bool ServiceRegistry::unregisterService(const std::string& serviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(serviceId);
    if (it != services_.end()) {
        services_.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<Service> ServiceRegistry::getService(const std::string& serviceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(serviceId);
    if (it != services_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::getServicesByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Service>> result;
    for (const auto& [id, service] : services_) {
        if (service->name == name) {
            result.push_back(service);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::getAllServices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Service>> result;
    result.reserve(services_.size());
    for (const auto& [id, service] : services_) {
        result.push_back(service);
    }
    return result;
}

bool ServiceRegistry::updateServiceStatus(const std::string& serviceId, const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(serviceId);
    if (it != services_.end()) {
        it->second->status = status;
        return true;
    }
    return false;
}

void ServiceRegistry::updateHeartbeat(const std::string& serviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = services_.find(serviceId);
    if (it != services_.end()) {
        it->second->lastHeartbeat = std::chrono::system_clock::now();
    }
}

std::vector<std::shared_ptr<Service>> ServiceRegistry::getHealthyServices(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Service>> result;
    for (const auto& [id, service] : services_) {
        if (service->name == name && service->status == "healthy") {
            result.push_back(service);
        }
    }
    return result;
}

} // namespace dcp