#pragma once
#include <memory>
#include "service_registry.h"
#include "health_checker.h"
#include "load_balancer.h"
#include "config_manager.h"
#include "monitoring.h"
#include "http_server.h"

namespace dcp {

class ControlPlane {
private:
    std::shared_ptr<ServiceRegistry> serviceRegistry_;
    std::shared_ptr<HealthChecker> healthChecker_;
    std::shared_ptr<LoadBalancer> loadBalancer_;
    std::shared_ptr<ConfigManager> configManager_;
    std::shared_ptr<Monitoring> monitoring_;
    std::shared_ptr<HttpServer> httpServer_;
    
    bool running_;
    
    void setupRoutes();
    
    // API Handlers
    HttpResponse handleGetServices(const HttpRequest& request);
    HttpResponse handleRegisterService(const HttpRequest& request);
    HttpResponse handleUnregisterService(const HttpRequest& request);
    HttpResponse handleGetMetrics(const HttpRequest& request);
    HttpResponse handleGetConfig(const HttpRequest& request);
    HttpResponse handleUpdateConfig(const HttpRequest& request);
    HttpResponse handleProxyRequest(const HttpRequest& request);
    HttpResponse handleDashboard(const HttpRequest& request);
    
public:
    ControlPlane(int port = 8080);
    ~ControlPlane();
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Component accessors
    std::shared_ptr<ServiceRegistry> getServiceRegistry() const { return serviceRegistry_; }
    std::shared_ptr<HealthChecker> getHealthChecker() const { return healthChecker_; }
    std::shared_ptr<LoadBalancer> getLoadBalancer() const { return loadBalancer_; }
    std::shared_ptr<ConfigManager> getConfigManager() const { return configManager_; }
    std::shared_ptr<Monitoring> getMonitoring() const { return monitoring_; }
    std::shared_ptr<HttpServer> getHttpServer() const { return httpServer_; }
    
    void waitForShutdown();
};

} // namespace dcp