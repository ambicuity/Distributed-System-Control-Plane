#include "control_plane.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace dcp {

ControlPlane::ControlPlane(int port) : running_(false) {
    serviceRegistry_ = std::make_shared<ServiceRegistry>();
    healthChecker_ = std::make_shared<HealthChecker>(serviceRegistry_);
    loadBalancer_ = std::make_shared<LoadBalancer>(serviceRegistry_);
    configManager_ = std::make_shared<ConfigManager>();
    monitoring_ = std::make_shared<Monitoring>();
    httpServer_ = std::make_shared<HttpServer>(port);
    
    setupRoutes();
}

ControlPlane::~ControlPlane() {
    stop();
}

bool ControlPlane::start() {
    if (running_) {
        return false;
    }
    
    std::cout << "Starting Distributed System Control Plane..." << std::endl;
    
    // Set static directory for web UI
    httpServer_->setStaticDirectory("web");
    
    // Start HTTP server
    if (!httpServer_->start()) {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return false;
    }
    
    // Start health checker
    healthChecker_->start();
    
    running_ = true;
    std::cout << "Control Plane started successfully on port " << httpServer_->getPort() << std::endl;
    
    return true;
}

void ControlPlane::stop() {
    if (!running_) {
        return;
    }
    
    std::cout << "Stopping Control Plane..." << std::endl;
    
    healthChecker_->stop();
    httpServer_->stop();
    
    running_ = false;
    std::cout << "Control Plane stopped" << std::endl;
}

void ControlPlane::setupRoutes() {
    // API Routes
    httpServer_->get("/api/services", [this](const HttpRequest& req) { 
        return handleGetServices(req); 
    });
    
    httpServer_->post("/api/services/register", [this](const HttpRequest& req) { 
        return handleRegisterService(req); 
    });
    
    httpServer_->post("/api/services/unregister", [this](const HttpRequest& req) { 
        return handleUnregisterService(req); 
    });
    
    httpServer_->get("/api/metrics", [this](const HttpRequest& req) { 
        return handleGetMetrics(req); 
    });
    
    httpServer_->get("/api/config", [this](const HttpRequest& req) { 
        return handleGetConfig(req); 
    });
    
    httpServer_->post("/api/config", [this](const HttpRequest& req) { 
        return handleUpdateConfig(req); 
    });
    
    httpServer_->get("/", [this](const HttpRequest& req) { 
        return handleDashboard(req); 
    });
    
    // Proxy route (catch-all for service requests)
    httpServer_->get("/proxy/*", [this](const HttpRequest& req) { 
        return handleProxyRequest(req); 
    });
}

HttpResponse ControlPlane::handleGetServices(const HttpRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    try {
        nlohmann::json result = nlohmann::json::array();
        auto services = serviceRegistry_->getAllServices();
        
        for (const auto& service : services) {
            nlohmann::json serviceJson;
            serviceJson["id"] = service->id;
            serviceJson["name"] = service->name;
            serviceJson["host"] = service->host;
            serviceJson["port"] = service->port;
            serviceJson["status"] = service->status;
            serviceJson["metadata"] = service->metadata;
            
            auto time_t = std::chrono::system_clock::to_time_t(service->lastHeartbeat);
            serviceJson["lastHeartbeat"] = time_t;
            
            result.push_back(serviceJson);
        }
        
        response.body = result.dump(4);
        monitoring_->recordRequestCount("/api/services", "GET");
        
    } catch (const std::exception& e) {
        response.status = 500;
        response.body = "{\"error\": \"" + std::string(e.what()) + "\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(endTime - startTime).count();
    monitoring_->recordRequestDuration("/api/services", duration);
    
    return response;
}

HttpResponse ControlPlane::handleRegisterService(const HttpRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    try {
        nlohmann::json requestJson = nlohmann::json::parse(request.body);
        
        std::string id = requestJson.value("id", "");
        std::string name = requestJson.value("name", "");
        std::string host = requestJson.value("host", "localhost");
        int port = requestJson.value("port", 0);
        
        if (id.empty() || name.empty() || port <= 0) {
            response.status = 400;
            response.body = "{\"error\": \"Missing required fields: id, name, port\"}";
            return response;
        }
        
        auto service = std::make_shared<Service>(id, name, host, port);
        
        // Set metadata if provided
        if (requestJson.contains("metadata")) {
            for (auto& [key, value] : requestJson["metadata"].items()) {
                service->metadata[key] = value.get<std::string>();
            }
        }
        
        if (serviceRegistry_->registerService(service)) {
            nlohmann::json result;
            result["success"] = true;
            result["message"] = "Service registered successfully";
            response.body = result.dump(4);
            
            std::cout << "Service registered: " << name << " (" << id << ") at " 
                     << host << ":" << port << std::endl;
        } else {
            response.status = 500;
            response.body = "{\"error\": \"Failed to register service\"}";
        }
        
        monitoring_->recordRequestCount("/api/services/register", "POST");
        
    } catch (const std::exception& e) {
        response.status = 400;
        response.body = "{\"error\": \"Invalid JSON: " + std::string(e.what()) + "\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(endTime - startTime).count();
    monitoring_->recordRequestDuration("/api/services/register", duration);
    
    return response;
}

HttpResponse ControlPlane::handleUnregisterService(const HttpRequest& request) {
    auto startTime = std::chrono::steady_clock::now();
    
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    try {
        nlohmann::json requestJson = nlohmann::json::parse(request.body);
        std::string id = requestJson.value("id", "");
        
        if (id.empty()) {
            response.status = 400;
            response.body = "{\"error\": \"Missing required field: id\"}";
            return response;
        }
        
        if (serviceRegistry_->unregisterService(id)) {
            nlohmann::json result;
            result["success"] = true;
            result["message"] = "Service unregistered successfully";
            response.body = result.dump(4);
            
            std::cout << "Service unregistered: " << id << std::endl;
        } else {
            response.status = 404;
            response.body = "{\"error\": \"Service not found\"}";
        }
        
        monitoring_->recordRequestCount("/api/services/unregister", "POST");
        
    } catch (const std::exception& e) {
        response.status = 400;
        response.body = "{\"error\": \"Invalid JSON: " + std::string(e.what()) + "\"}";
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(endTime - startTime).count();
    monitoring_->recordRequestDuration("/api/services/unregister", duration);
    
    return response;
}

HttpResponse ControlPlane::handleGetMetrics(const HttpRequest& request) {
    HttpResponse response;
    
    std::string format = request.params.count("format") ? request.params.at("format") : "prometheus";
    
    if (format == "json") {
        response.headers["Content-Type"] = "application/json";
        response.body = monitoring_->exportMetricsJson();
    } else {
        response.headers["Content-Type"] = "text/plain";
        response.body = monitoring_->exportMetrics();
    }
    
    return response;
}

HttpResponse ControlPlane::handleGetConfig(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.body = configManager_->toString();
    return response;
}

HttpResponse ControlPlane::handleUpdateConfig(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    
    try {
        nlohmann::json configJson = nlohmann::json::parse(request.body);
        
        for (auto& [section, values] : configJson.items()) {
            configManager_->setSection(section, values);
        }
        
        configManager_->saveConfig();
        
        nlohmann::json result;
        result["success"] = true;
        result["message"] = "Configuration updated successfully";
        response.body = result.dump(4);
        
    } catch (const std::exception& e) {
        response.status = 400;
        response.body = "{\"error\": \"" + std::string(e.what()) + "\"}";
    }
    
    return response;
}

HttpResponse ControlPlane::handleProxyRequest(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";
    response.status = 501;
    response.body = "{\"error\": \"Proxy functionality not implemented in this demo\"}";
    return response;
}

HttpResponse ControlPlane::handleDashboard(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "text/html";
    
    // Simple embedded dashboard HTML
    response.body = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>Distributed System Control Plane</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .header { background: #333; color: white; padding: 20px; border-radius: 5px; }
        .container { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-top: 20px; }
        .panel { background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .service { background: #e9f5ff; padding: 10px; margin: 10px 0; border-radius: 3px; border-left: 4px solid #007acc; }
        .service.healthy { border-left-color: #28a745; }
        .service.unhealthy { border-left-color: #dc3545; }
        .metrics { font-family: monospace; background: #f8f9fa; padding: 15px; border-radius: 3px; }
        button { background: #007acc; color: white; border: none; padding: 10px 20px; border-radius: 3px; cursor: pointer; margin: 5px; }
        button:hover { background: #005c99; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Distributed System Control Plane</h1>
        <p>Manage and monitor your distributed services</p>
    </div>
    
    <div class="container">
        <div class="panel">
            <h2>Services</h2>
            <div id="services">Loading...</div>
            <button onclick="refreshServices()">Refresh</button>
        </div>
        
        <div class="panel">
            <h2>Metrics</h2>
            <div id="metrics" class="metrics">Loading...</div>
            <button onclick="refreshMetrics()">Refresh</button>
        </div>
    </div>
    
    <div class="panel" style="margin-top: 20px;">
        <h2>Register New Service</h2>
        <form onsubmit="registerService(event)">
            <input type="text" id="serviceId" placeholder="Service ID" required style="margin: 5px; padding: 8px;">
            <input type="text" id="serviceName" placeholder="Service Name" required style="margin: 5px; padding: 8px;">
            <input type="text" id="serviceHost" placeholder="Host" value="localhost" style="margin: 5px; padding: 8px;">
            <input type="number" id="servicePort" placeholder="Port" required style="margin: 5px; padding: 8px;">
            <button type="submit">Register Service</button>
        </form>
    </div>
    
    <script>
        function refreshServices() {
            fetch('/api/services')
                .then(response => response.json())
                .then(services => {
                    const container = document.getElementById('services');
                    if (services.length === 0) {
                        container.innerHTML = '<p>No services registered</p>';
                        return;
                    }
                    container.innerHTML = services.map(service => 
                        '<div class="service ' + service.status + '">' +
                        '<strong>' + service.name + '</strong> (' + service.id + ')<br>' +
                        'Location: ' + service.host + ':' + service.port + '<br>' +
                        'Status: ' + service.status.toUpperCase() +
                        '</div>'
                    ).join('');
                })
                .catch(error => {
                    document.getElementById('services').innerHTML = '<p>Error loading services</p>';
                    console.error('Error:', error);
                });
        }
        
        function refreshMetrics() {
            fetch('/api/metrics?format=json')
                .then(response => response.json())
                .then(metrics => {
                    const container = document.getElementById('metrics');
                    if (metrics.length === 0) {
                        container.innerHTML = 'No metrics available';
                        return;
                    }
                    container.innerHTML = metrics.map(metric => 
                        metric.name + ': ' + metric.value + ' (' + metric.type + ')'
                    ).join('\n');
                })
                .catch(error => {
                    document.getElementById('metrics').innerHTML = 'Error loading metrics';
                    console.error('Error:', error);
                });
        }
        
        function registerService(event) {
            event.preventDefault();
            const data = {
                id: document.getElementById('serviceId').value,
                name: document.getElementById('serviceName').value,
                host: document.getElementById('serviceHost').value,
                port: parseInt(document.getElementById('servicePort').value)
            };
            
            fetch('/api/services/register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            })
            .then(response => response.json())
            .then(result => {
                if (result.success) {
                    alert('Service registered successfully!');
                    document.querySelector('form').reset();
                    refreshServices();
                } else {
                    alert('Error: ' + result.error);
                }
            })
            .catch(error => {
                alert('Error registering service');
                console.error('Error:', error);
            });
        }
        
        setInterval(function() {
            refreshServices();
            refreshMetrics();
        }, 10000);
        
        refreshServices();
        refreshMetrics();
    </script>
</body>
</html>)HTML";
    
    return response;
}

void ControlPlane::waitForShutdown() {
    std::cout << "Control Plane running. Press Ctrl+C to stop." << std::endl;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace dcp