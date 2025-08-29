# Distributed System Control Plane

A robust and scalable control plane for managing distributed services, built with C++ and designed for high performance and observability.

## Overview

This distributed system control plane provides:
- **Service Discovery & Registry**: Register, discover, and manage distributed services
- **Health Monitoring**: Continuous health checks with configurable intervals
- **Load Balancing**: Multiple algorithms (Round Robin, Random, etc.)
- **Configuration Management**: Centralized configuration with dynamic updates
- **Observability**: Metrics collection and monitoring integration
- **Web Dashboard**: Intuitive UI for managing services and viewing metrics
- **RESTful API**: Complete API for programmatic control

## Architecture

The control plane consists of several key components:

- **Control Plane Server**: Main orchestration engine
- **Service Registry**: Tracks and manages service instances
- **Health Checker**: Monitors service health continuously
- **Load Balancer**: Distributes traffic across healthy services
- **Configuration Manager**: Handles dynamic configuration
- **HTTP Server**: Serves API and web dashboard
- **Monitoring System**: Collects and exports metrics

## Features

### 🚀 Core Capabilities
- Service registration and discovery
- Automatic health monitoring
- Load balancing with multiple algorithms
- Real-time metrics collection
- Configuration management
- Web-based dashboard

### 📊 Observability
- Prometheus-compatible metrics export
- JSON metrics API
- Service health tracking
- Request monitoring
- Performance metrics

### 🌐 User Interface
- Modern web dashboard
- Real-time service status
- Interactive service registration
- Metrics visualization
- Responsive design

## Build and Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.12+
- Linux/Unix environment

### Building
```bash
mkdir build
cd build
cmake ..
make

# Executables will be in build/bin/
```

### Installation
```bash
make install
```

## Usage

### Starting the Control Plane
```bash
# Start on default port 8080
./bin/control-plane

# Start on custom port
./bin/control-plane 9000
```

### Accessing the Dashboard
Open your browser and navigate to:
- **Dashboard**: http://localhost:8080
- **API**: http://localhost:8080/api
- **Metrics**: http://localhost:8080/api/metrics

### Running Example Services
```bash
# Start example services
./bin/example-service svc001 "User Service" 9001
./bin/example-service svc002 "Order Service" 9002
./bin/example-service svc003 "Payment Service" 9003
```

### Registering Services via API
```bash
# Register a service
curl -X POST http://localhost:8080/api/services/register \
  -H 'Content-Type: application/json' \
  -d '{
    "id": "user-service-01",
    "name": "UserService",
    "host": "localhost",
    "port": 9001,
    "metadata": {
      "version": "1.0.0",
      "environment": "development"
    }
  }'

# List all services
curl http://localhost:8080/api/services

# Get metrics
curl http://localhost:8080/api/metrics
curl http://localhost:8080/api/metrics?format=json
```

## API Reference

### Services API

#### List Services
```http
GET /api/services
```

#### Register Service
```http
POST /api/services/register
Content-Type: application/json

{
  "id": "service-id",
  "name": "ServiceName",
  "host": "hostname",
  "port": 8080,
  "metadata": {}
}
```

#### Unregister Service
```http
POST /api/services/unregister
Content-Type: application/json

{
  "id": "service-id"
}
```

### Metrics API

#### Get Metrics (Prometheus format)
```http
GET /api/metrics
```

#### Get Metrics (JSON format)
```http
GET /api/metrics?format=json
```

### Configuration API

#### Get Configuration
```http
GET /api/config
```

#### Update Configuration
```http
POST /api/config
Content-Type: application/json

{
  "health_check": {
    "interval_ms": 30000
  },
  "load_balancer": {
    "algorithm": "round_robin"
  }
}
```

## Configuration

The control plane uses a `config.json` file for configuration:

```json
{
  "server": {
    "port": 8080,
    "host": "0.0.0.0"
  },
  "health_check": {
    "interval_ms": 30000,
    "timeout_ms": 5000
  },
  "load_balancer": {
    "algorithm": "round_robin"
  },
  "monitoring": {
    "enabled": true,
    "export_interval_ms": 10000
  }
}
```

## Monitoring and Metrics

The control plane exposes metrics in both Prometheus and JSON formats:

### Available Metrics
- `http_requests_total`: Total HTTP requests (counter)
- `http_request_duration_seconds`: Request duration (histogram)
- `service_health`: Service health status (gauge)

### Integration with Monitoring Tools
- **Prometheus**: Scrape `/api/metrics`
- **Grafana**: Create dashboards using the metrics
- **Custom Tools**: Use `/api/metrics?format=json`

## Distributed System Concepts Demonstrated

This control plane demonstrates key distributed system patterns:

1. **Service Discovery**: Dynamic service registration and discovery
2. **Health Checking**: Continuous monitoring with failure detection
3. **Load Balancing**: Traffic distribution across service instances
4. **Circuit Breaker**: Failure isolation (basic implementation)
5. **Configuration Management**: Centralized configuration distribution
6. **Observability**: Metrics, logging, and monitoring
7. **API Gateway**: Single entry point for service access
8. **Scalability**: Designed to handle multiple services and requests

## Development

### Project Structure
```
├── include/          # Header files
├── src/              # Source files
├── examples/         # Example services
├── web/              # Web dashboard files
├── third_party/      # External dependencies
├── CMakeLists.txt    # Build configuration
└── README.md
```

### Adding New Features
1. Create header files in `include/`
2. Implement in `src/`
3. Update CMakeLists.txt if needed
4. Add tests and documentation

## Troubleshooting

### Common Issues
1. **Port already in use**: Change the port or stop the conflicting service
2. **Permission denied**: Ensure proper file permissions
3. **Service not appearing**: Check health endpoint is accessible

### Debug Mode
Enable debug logging by setting environment variable:
```bash
export DEBUG=1
./bin/control-plane
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

---

*Built with ❤️ for distributed systems*