#include "health_checker.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace dcp {

HealthChecker::HealthChecker(std::shared_ptr<ServiceRegistry> registry, int checkIntervalMs)
    : registry_(registry), running_(false), checkIntervalMs_(checkIntervalMs) {
}

HealthChecker::~HealthChecker() {
    stop();
}

void HealthChecker::start() {
    if (running_.exchange(true)) {
        return; // Already running
    }
    
    checkerThread_ = std::thread([this]() { checkServicesHealth(); });
}

void HealthChecker::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    if (checkerThread_.joinable()) {
        checkerThread_.join();
    }
}

void HealthChecker::checkServicesHealth() {
    while (running_) {
        auto services = registry_->getAllServices();
        
        for (const auto& service : services) {
            if (!running_) break;
            
            bool isHealthy = performHealthCheck(service);
            std::string newStatus = isHealthy ? "healthy" : "unhealthy";
            
            if (service->status != newStatus) {
                registry_->updateServiceStatus(service->id, newStatus);
                std::cout << "Service " << service->name << " (" << service->id 
                         << ") status changed to: " << newStatus << std::endl;
            }
        }
        
        // Sleep for the check interval
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs_));
    }
}

bool HealthChecker::performHealthCheck(const std::shared_ptr<Service>& service) {
    // Simple TCP connection check to the service
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }
    
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 second timeout
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(service->port);
    
    if (inet_pton(AF_INET, service->host.c_str(), &serverAddr.sin_addr) <= 0) {
        close(sockfd);
        return false;
    }
    
    int result = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    close(sockfd);
    
    if (result == 0) {
        registry_->updateHeartbeat(service->id);
        return true;
    }
    
    return false;
}

} // namespace dcp