#include "load_balancer.h"
#include <algorithm>

namespace dcp {

LoadBalancer::LoadBalancer(std::shared_ptr<ServiceRegistry> registry, LoadBalancingAlgorithm algorithm)
    : registry_(registry), algorithm_(algorithm), roundRobinCounter_(0), randomGenerator_(randomDevice_()) {
}

std::shared_ptr<Service> LoadBalancer::selectService(const std::string& serviceName) const {
    auto services = registry_->getHealthyServices(serviceName);
    
    if (services.empty()) {
        return nullptr;
    }
    
    switch (algorithm_) {
        case LoadBalancingAlgorithm::ROUND_ROBIN:
            return selectRoundRobin(services);
        case LoadBalancingAlgorithm::RANDOM:
            return selectRandom(services);
        case LoadBalancingAlgorithm::LEAST_CONNECTIONS:
            // For now, fallback to round robin
            // In a real implementation, we'd track connection counts
            return selectRoundRobin(services);
        case LoadBalancingAlgorithm::WEIGHTED_ROUND_ROBIN:
            // For now, fallback to round robin
            // In a real implementation, we'd use service weights
            return selectRoundRobin(services);
        default:
            return selectRoundRobin(services);
    }
}

std::shared_ptr<Service> LoadBalancer::selectRoundRobin(const std::vector<std::shared_ptr<Service>>& services) const {
    if (services.empty()) {
        return nullptr;
    }
    
    size_t index = roundRobinCounter_.fetch_add(1) % services.size();
    return services[index];
}

std::shared_ptr<Service> LoadBalancer::selectRandom(const std::vector<std::shared_ptr<Service>>& services) const {
    if (services.empty()) {
        return nullptr;
    }
    
    std::uniform_int_distribution<size_t> distribution(0, services.size() - 1);
    size_t index = distribution(randomGenerator_);
    return services[index];
}

} // namespace dcp