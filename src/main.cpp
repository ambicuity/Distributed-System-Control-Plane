#include "control_plane.h"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<dcp::ControlPlane> controlPlane;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    if (controlPlane) {
        controlPlane->stop();
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    std::cout << "=== Distributed System Control Plane ===" << std::endl;
    std::cout << "A robust and scalable control plane for managing distributed services" << std::endl;
    std::cout << std::endl;
    
    try {
        controlPlane = std::make_unique<dcp::ControlPlane>(port);
        
        if (!controlPlane->start()) {
            std::cerr << "Failed to start control plane" << std::endl;
            return 1;
        }
        
        std::cout << "🌐 Web Dashboard: http://localhost:" << port << std::endl;
        std::cout << "📊 API Endpoint: http://localhost:" << port << "/api" << std::endl;
        std::cout << "📈 Metrics: http://localhost:" << port << "/api/metrics" << std::endl;
        std::cout << std::endl;
        
        // Wait for shutdown
        controlPlane->waitForShutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}