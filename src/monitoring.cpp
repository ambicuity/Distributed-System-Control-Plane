#include "monitoring.h"
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace dcp {

void Monitoring::incrementCounter(const std::string& name, double value, 
                                 const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = name;
    if (!labels.empty()) {
        key += "{";
        bool first = true;
        for (const auto& [k, v] : labels) {
            if (!first) key += ",";
            key += k + "=" + v;
            first = false;
        }
        key += "}";
    }
    
    auto it = metrics_.find(key);
    if (it == metrics_.end()) {
        auto metric = std::make_shared<Metric>(name, "counter");
        metric->labels = labels;
        metrics_[key] = metric;
        it = metrics_.find(key);
    }
    
    it->second->value.store(it->second->value.load() + value);
    it->second->timestamp = std::chrono::system_clock::now();
}

void Monitoring::setGauge(const std::string& name, double value, 
                         const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = name;
    if (!labels.empty()) {
        key += "{";
        bool first = true;
        for (const auto& [k, v] : labels) {
            if (!first) key += ",";
            key += k + "=" + v;
            first = false;
        }
        key += "}";
    }
    
    auto it = metrics_.find(key);
    if (it == metrics_.end()) {
        auto metric = std::make_shared<Metric>(name, "gauge");
        metric->labels = labels;
        metrics_[key] = metric;
        it = metrics_.find(key);
    }
    
    it->second->value.store(value);
    it->second->timestamp = std::chrono::system_clock::now();
}

void Monitoring::recordHistogram(const std::string& name, double value, 
                                const std::unordered_map<std::string, std::string>& labels) {
    // For simplicity, treat histograms as gauges for now
    // In a real implementation, we'd maintain buckets and counts
    setGauge(name, value, labels);
}

std::shared_ptr<Metric> Monitoring::getMetric(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Metric>> Monitoring::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Metric>> result;
    result.reserve(metrics_.size());
    for (const auto& [key, metric] : metrics_) {
        result.push_back(metric);
    }
    return result;
}

std::string Monitoring::exportMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    for (const auto& [key, metric] : metrics_) {
        // Prometheus format
        ss << "# TYPE " << metric->name << " " << metric->type << "\n";
        
        if (metric->labels.empty()) {
            ss << metric->name << " " << std::fixed << std::setprecision(6) << metric->value.load() << "\n";
        } else {
            ss << metric->name << "{";
            bool first = true;
            for (const auto& [k, v] : metric->labels) {
                if (!first) ss << ",";
                ss << k << "=\"" << v << "\"";
                first = false;
            }
            ss << "} " << std::fixed << std::setprecision(6) << metric->value.load() << "\n";
        }
    }
    
    return ss.str();
}

std::string Monitoring::exportMetricsJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json result = nlohmann::json::array();
    
    for (const auto& [key, metric] : metrics_) {
        nlohmann::json metricJson;
        metricJson["name"] = metric->name;
        metricJson["type"] = metric->type;
        metricJson["value"] = metric->value.load();
        metricJson["labels"] = metric->labels;
        
        auto time_t = std::chrono::system_clock::to_time_t(metric->timestamp);
        metricJson["timestamp"] = time_t;
        
        result.push_back(metricJson);
    }
    
    return result.dump(4);
}

void Monitoring::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
}

void Monitoring::recordRequestCount(const std::string& endpoint, const std::string& method) {
    std::unordered_map<std::string, std::string> labels = {
        {"endpoint", endpoint},
        {"method", method}
    };
    incrementCounter("http_requests_total", 1.0, labels);
}

void Monitoring::recordRequestDuration(const std::string& endpoint, double duration) {
    std::unordered_map<std::string, std::string> labels = {
        {"endpoint", endpoint}
    };
    recordHistogram("http_request_duration_seconds", duration, labels);
}

void Monitoring::recordServiceHealth(const std::string& serviceName, bool healthy) {
    std::unordered_map<std::string, std::string> labels = {
        {"service", serviceName}
    };
    setGauge("service_health", healthy ? 1.0 : 0.0, labels);
}

} // namespace dcp