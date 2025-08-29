#pragma once
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include "service_registry.h"

namespace dcp {

class HealthChecker {
private:
    std::shared_ptr<ServiceRegistry> registry_;
    std::atomic<bool> running_;
    std::thread checkerThread_;
    int checkIntervalMs_;
    
    void checkServicesHealth();
    bool performHealthCheck(const std::shared_ptr<Service>& service);
    
public:
    HealthChecker(std::shared_ptr<ServiceRegistry> registry, 
                  int checkIntervalMs = 30000);
    ~HealthChecker();
    
    void start();
    void stop();
    bool isRunning() const { return running_; }
};

} // namespace dcp