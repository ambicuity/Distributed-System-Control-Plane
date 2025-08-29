#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <functional>
#include <mutex>

namespace dcp {

struct Service {
    std::string id;
    std::string name;
    std::string host;
    int port;
    std::string status; // "healthy", "unhealthy", "unknown"
    std::unordered_map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point lastHeartbeat;
    
    Service(const std::string& id, const std::string& name, 
            const std::string& host, int port)
        : id(id), name(name), host(host), port(port), 
          status("unknown"), lastHeartbeat(std::chrono::system_clock::now()) {}
};

class ServiceRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<Service>> services_;
    mutable std::mutex mutex_;

public:
    bool registerService(const std::shared_ptr<Service>& service);
    bool unregisterService(const std::string& serviceId);
    std::shared_ptr<Service> getService(const std::string& serviceId) const;
    std::vector<std::shared_ptr<Service>> getServicesByName(const std::string& name) const;
    std::vector<std::shared_ptr<Service>> getAllServices() const;
    bool updateServiceStatus(const std::string& serviceId, const std::string& status);
    void updateHeartbeat(const std::string& serviceId);
    std::vector<std::shared_ptr<Service>> getHealthyServices(const std::string& name) const;
};

} // namespace dcp