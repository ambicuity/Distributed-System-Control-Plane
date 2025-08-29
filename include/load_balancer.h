#pragma once
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <atomic>
#include "service_registry.h"

namespace dcp {

enum class LoadBalancingAlgorithm {
    ROUND_ROBIN,
    RANDOM,
    LEAST_CONNECTIONS,
    WEIGHTED_ROUND_ROBIN
};

class LoadBalancer {
private:
    std::shared_ptr<ServiceRegistry> registry_;
    LoadBalancingAlgorithm algorithm_;
    mutable std::atomic<size_t> roundRobinCounter_;
    mutable std::random_device randomDevice_;
    mutable std::mt19937 randomGenerator_;
    
public:
    LoadBalancer(std::shared_ptr<ServiceRegistry> registry, 
                 LoadBalancingAlgorithm algorithm = LoadBalancingAlgorithm::ROUND_ROBIN);
    
    std::shared_ptr<Service> selectService(const std::string& serviceName) const;
    void setAlgorithm(LoadBalancingAlgorithm algorithm) { algorithm_ = algorithm; }
    LoadBalancingAlgorithm getAlgorithm() const { return algorithm_; }
    
private:
    std::shared_ptr<Service> selectRoundRobin(const std::vector<std::shared_ptr<Service>>& services) const;
    std::shared_ptr<Service> selectRandom(const std::vector<std::shared_ptr<Service>>& services) const;
};

} // namespace dcp