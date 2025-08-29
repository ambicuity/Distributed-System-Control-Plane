cmake_minimum_required(VERSION 3.12)
project(DistributedSystemControlPlane VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(CONTROL_PLANE_SOURCES
    src/main.cpp
    src/control_plane.cpp
    src/service_registry.cpp
    src/health_checker.cpp
    src/load_balancer.cpp
    src/config_manager.cpp
    src/monitoring.cpp
    src/http_server.cpp
)

# Create the main executable
add_executable(control-plane ${CONTROL_PLANE_SOURCES})

# Link libraries
target_link_libraries(control-plane Threads::Threads)

# Example service executable
add_executable(example-service 
    examples/example_service.cpp
    src/http_server.cpp
)
target_link_libraries(example-service Threads::Threads)

# Set output directory
set_target_properties(control-plane PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(example-service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS control-plane example-service
    RUNTIME DESTINATION bin
)

# Install web UI files
install(DIRECTORY web/
    DESTINATION share/web
    FILES_MATCHING PATTERN "*.html" PATTERN "*.css" PATTERN "*.js"
)