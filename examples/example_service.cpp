#include "http_server.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <csignal>
#include <random>
#include <thread>
#include <chrono>

class ExampleService {
private:
    std::unique_ptr<dcp::HttpServer> server_;
    std::string serviceId_;
    std::string serviceName_;
    int port_;
    bool running_;
    
public:
    ExampleService(const std::string& serviceId, const std::string& serviceName, int port)
        : serviceId_(serviceId), serviceName_(serviceName), port_(port), running_(false) {
        server_ = std::make_unique<dcp::HttpServer>(port);
        setupRoutes();
    }
    
    void setupRoutes() {
        // Health check endpoint
        server_->get("/health", [this](const dcp::HttpRequest& req) {
            dcp::HttpResponse response;
            response.headers["Content-Type"] = "application/json";
            
            nlohmann::json health;
            health["service"] = serviceName_;
            health["id"] = serviceId_;
            health["status"] = "healthy";
            health["timestamp"] = std::time(nullptr);
            
            response.body = health.dump(4);
            return response;
        });
        
        // API endpoint that does some "work"
        server_->get("/api/process", [this](const dcp::HttpRequest& req) {
            dcp::HttpResponse response;
            response.headers["Content-Type"] = "application/json";
            
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(100 + std::rand() % 200));
            
            nlohmann::json result;
            result["service"] = serviceName_;
            result["id"] = serviceId_;
            result["message"] = "Request processed successfully";
            result["data"] = {
                {"processed_at", std::time(nullptr)},
                {"random_value", std::rand() % 1000}
            };
            
            response.body = result.dump(4);
            return response;
        });
        
        // Info endpoint
        server_->get("/", [this](const dcp::HttpRequest& req) {
            dcp::HttpResponse response;
            response.body = "<html><head><title>" + serviceName_ + "</title></head>"
                          "<body><h1>" + serviceName_ + "</h1>"
                          "<p>Service ID: " + serviceId_ + "</p>"
                          "<p>Port: " + std::to_string(port_) + "</p>"
                          "<p>Status: Running</p>"
                          "<p><a href='/health'>Health Check</a></p>"
                          "<p><a href='/api/process'>Process Request</a></p>"
                          "</body></html>";
            return response;
        });
    }
    
    bool start() {
        if (server_->start()) {
            running_ = true;
            std::cout << serviceName_ << " (" << serviceId_ << ") started on port " << port_ << std::endl;
            return true;
        }
        return false;
    }
    
    void stop() {
        if (running_) {
            server_->stop();
            running_ = false;
            std::cout << serviceName_ << " stopped" << std::endl;
        }
    }
    
    bool isRunning() const { return running_; }
    
    void registerWithControlPlane(const std::string& controlPlaneUrl) {
        // In a real implementation, this would make an HTTP POST to register
        std::cout << "To register this service with the control plane, use:" << std::endl;
        std::cout << "curl -X POST " << controlPlaneUrl << "/api/services/register \\" << std::endl;
        std::cout << "  -H 'Content-Type: application/json' \\" << std::endl;
        std::cout << "  -d '{" << std::endl;
        std::cout << "    \"id\": \"" << serviceId_ << "\"," << std::endl;
        std::cout << "    \"name\": \"" << serviceName_ << "\"," << std::endl;
        std::cout << "    \"host\": \"localhost\"," << std::endl;
        std::cout << "    \"port\": " << port_ << std::endl;
        std::cout << "  }'" << std::endl;
        std::cout << std::endl;
    }
};

std::unique_ptr<ExampleService> service;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    if (service) {
        service->stop();
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <service-id> <service-name> <port> [control-plane-url]" << std::endl;
        std::cout << "Example: " << argv[0] << " svc001 UserService 9001 http://localhost:8080" << std::endl;
        return 1;
    }
    
    std::string serviceId = argv[1];
    std::string serviceName = argv[2];
    int port = std::stoi(argv[3]);
    std::string controlPlaneUrl = argc > 4 ? argv[4] : "http://localhost:8080";
    
    std::cout << "=== Example Service ===" << std::endl;
    std::cout << "Service ID: " << serviceId << std::endl;
    std::cout << "Service Name: " << serviceName << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << std::endl;
    
    try {
        service = std::make_unique<ExampleService>(serviceId, serviceName, port);
        
        if (!service->start()) {
            std::cerr << "Failed to start service" << std::endl;
            return 1;
        }
        
        std::cout << "🌐 Service URL: http://localhost:" << port << std::endl;
        std::cout << "❤️  Health Check: http://localhost:" << port << "/health" << std::endl;
        std::cout << "⚙️  API Endpoint: http://localhost:" << port << "/api/process" << std::endl;
        std::cout << std::endl;
        
        // Show registration command
        service->registerWithControlPlane(controlPlaneUrl);
        
        // Keep running
        while (service->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}