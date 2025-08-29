#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <memory>

namespace dcp {

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::unordered_map<std::string, std::string> params;
};

struct HttpResponse {
    int status = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() {
        headers["Content-Type"] = "text/html";
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
private:
    int port_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    std::unordered_map<std::string, HttpHandler> routes_;
    std::string staticDir_;
    
    void serverLoop();
    void handleConnection(int clientSocket);
    HttpRequest parseRequest(const std::string& requestStr);
    std::string buildResponse(const HttpResponse& response);
    HttpResponse handleStaticFile(const std::string& path);
    
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    void addRoute(const std::string& method, const std::string& path, HttpHandler handler);
    void setStaticDirectory(const std::string& dir) { staticDir_ = dir; }
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    int getPort() const { return port_; }
    
    // Convenience methods for common HTTP methods
    void get(const std::string& path, HttpHandler handler) { 
        addRoute("GET", path, handler); 
    }
    void post(const std::string& path, HttpHandler handler) { 
        addRoute("POST", path, handler); 
    }
    void put(const std::string& path, HttpHandler handler) { 
        addRoute("PUT", path, handler); 
    }
    void del(const std::string& path, HttpHandler handler) { 
        addRoute("DELETE", path, handler); 
    }
};

} // namespace dcp