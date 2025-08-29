#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <mutex>
#include <memory>
#include <vector>

namespace dcp {

struct Metric {
    std::string name;
    std::string type; // "counter", "gauge", "histogram"
    std::atomic<double> value;
    std::unordered_map<std::string, std::string> labels;
    std::chrono::system_clock::time_point timestamp;
    
    Metric(const std::string& name, const std::string& type) 
        : name(name), type(type), value(0.0), timestamp(std::chrono::system_clock::now()) {}
};

class Monitoring {
private:
    std::unordered_map<std::string, std::shared_ptr<Metric>> metrics_;
    mutable std::mutex mutex_;
    
public:
    void incrementCounter(const std::string& name, double value = 1.0,
                         const std::unordered_map<std::string, std::string>& labels = {});
    void setGauge(const std::string& name, double value,
                  const std::unordered_map<std::string, std::string>& labels = {});
    void recordHistogram(const std::string& name, double value,
                        const std::unordered_map<std::string, std::string>& labels = {});
    
    std::shared_ptr<Metric> getMetric(const std::string& name) const;
    std::vector<std::shared_ptr<Metric>> getAllMetrics() const;
    
    std::string exportMetrics() const; // Prometheus format
    std::string exportMetricsJson() const;
    
    void reset();
    
    // Convenience methods for common metrics
    void recordRequestCount(const std::string& endpoint, const std::string& method);
    void recordRequestDuration(const std::string& endpoint, double duration);
    void recordServiceHealth(const std::string& serviceName, bool healthy);
};

} // namespace dcp