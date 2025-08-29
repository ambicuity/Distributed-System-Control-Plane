#include "http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace dcp {

HttpServer::HttpServer(int port) : port_(port), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::addRoute(const std::string& method, const std::string& path, HttpHandler handler) {
    std::string key = method + " " + path;
    routes_[key] = handler;
}

bool HttpServer::start() {
    if (running_.exchange(true)) {
        return false; // Already running
    }
    
    serverThread_ = std::thread([this]() { serverLoop(); });
    
    // Give the server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return running_;
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }
    
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void HttpServer::serverLoop() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        running_ = false;
        return;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding to port " << port_ << std::endl;
        close(serverSocket);
        running_ = false;
        return;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        running_ = false;
        return;
    }
    
    std::cout << "HTTP Server started on port " << port_ << std::endl;
    
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
            }
            continue;
        }
        
        // Handle connection in a separate thread for better concurrency
        std::thread([this, clientSocket]() {
            handleConnection(clientSocket);
        }).detach();
    }
    
    close(serverSocket);
    std::cout << "HTTP Server stopped" << std::endl;
}

void HttpServer::handleConnection(int clientSocket) {
    char buffer[4096] = {0};
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    std::string requestStr(buffer, bytesRead);
    HttpRequest request = parseRequest(requestStr);
    
    HttpResponse response;
    std::string routeKey = request.method + " " + request.path;
    
    auto it = routes_.find(routeKey);
    if (it != routes_.end()) {
        try {
            response = it->second(request);
        } catch (const std::exception& e) {
            response.status = 500;
            response.body = "Internal Server Error: " + std::string(e.what());
        }
    } else if (request.method == "GET" && !staticDir_.empty()) {
        // Try to serve static file
        response = handleStaticFile(request.path);
    } else {
        response.status = 404;
        response.body = "Not Found";
    }
    
    std::string responseStr = buildResponse(response);
    send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
    
    close(clientSocket);
}

HttpRequest HttpServer::parseRequest(const std::string& requestStr) {
    HttpRequest request;
    
    std::istringstream stream(requestStr);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path;
        
        // Extract query parameters
        size_t queryPos = request.path.find('?');
        if (queryPos != std::string::npos) {
            std::string query = request.path.substr(queryPos + 1);
            request.path = request.path.substr(0, queryPos);
            
            // Parse query parameters (simple implementation)
            std::istringstream queryStream(query);
            std::string param;
            while (std::getline(queryStream, param, '&')) {
                size_t eqPos = param.find('=');
                if (eqPos != std::string::npos) {
                    std::string key = param.substr(0, eqPos);
                    std::string value = param.substr(eqPos + 1);
                    request.params[key] = value;
                }
            }
        }
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            request.headers[key] = value;
        }
    }
    
    // Parse body (rest of the request)
    std::string body;
    std::string bodyLine;
    while (std::getline(stream, bodyLine)) {
        body += bodyLine + "\n";
    }
    if (!body.empty()) {
        body.pop_back(); // Remove last newline
    }
    request.body = body;
    
    return request;
}

std::string HttpServer::buildResponse(const HttpResponse& response) {
    std::stringstream ss;
    
    ss << "HTTP/1.1 " << response.status << " ";
    switch (response.status) {
        case 200: ss << "OK"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default: ss << "Unknown"; break;
    }
    ss << "\r\n";
    
    // Add headers
    for (const auto& [key, value] : response.headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    ss << "Content-Length: " << response.body.length() << "\r\n";
    ss << "\r\n";
    ss << response.body;
    
    return ss.str();
}

HttpResponse HttpServer::handleStaticFile(const std::string& path) {
    HttpResponse response;
    
    std::string filePath = staticDir_ + path;
    if (path == "/") {
        filePath += "index.html";
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        response.status = 404;
        response.body = "File not found";
        return response;
    }
    
    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    response.body = buffer.str();
    
    // Set content type based on file extension
    if (filePath.length() >= 5 && filePath.substr(filePath.length() - 5) == ".html") {
        response.headers["Content-Type"] = "text/html";
    } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".css") {
        response.headers["Content-Type"] = "text/css";
    } else if (filePath.length() >= 3 && filePath.substr(filePath.length() - 3) == ".js") {
        response.headers["Content-Type"] = "application/javascript";
    } else if (filePath.length() >= 5 && filePath.substr(filePath.length() - 5) == ".json") {
        response.headers["Content-Type"] = "application/json";
    } else {
        response.headers["Content-Type"] = "application/octet-stream";
    }
    
    return response;
}

} // namespace dcp