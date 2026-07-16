#include "visualization_service.h"

#include <chrono>
#include <iterator>
#include <utility>

namespace keyrecord {
namespace {

constexpr auto API_CACHE_TTL = std::chrono::seconds(2);
constexpr std::size_t MAX_API_CACHE_ENTRIES = 128;

bool isCacheableApiRequest(std::string_view method, std::string_view target) {
    return method == "GET" && target.rfind("/api/", 0) == 0;
}

std::string cacheKey(std::string_view method, std::string_view target) {
    std::string key;
    key.reserve(method.size() + target.size() + 1);
    key.append(method);
    key.push_back(' ');
    key.append(target);
    return key;
}

} // namespace

VisualizationService::VisualizationService(ReadOnlyDatabase database)
    : database_(std::move(database)) {
}

VisualizationService::VisualizationService(VisualizationService&& other) noexcept
    : database_(std::move(other.database_)) {
}

VisualizationService& VisualizationService::operator=(VisualizationService&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(cacheMutex_, other.cacheMutex_);
        database_ = std::move(other.database_);
        cacheRecency_.clear();
        apiCache_.clear();
    }
    return *this;
}

std::optional<VisualizationService> VisualizationService::open(const std::string& dbPath, std::string* errorMessage) {
    auto database = ReadOnlyDatabase::open(dbPath, errorMessage);
    if (!database) {
        return std::nullopt;
    }

    return VisualizationService(std::move(*database));
}

HttpResponse VisualizationService::handleRequest(std::string_view method, std::string_view target) const {
    if (!isCacheableApiRequest(method, target)) {
        return handleHttpRequest(method, target, database_.get());
    }

    const std::string key = cacheKey(method, target);
    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        const auto it = apiCache_.find(key);
        if (it != apiCache_.end()) {
            if (it->second.expiresAt > now) {
                cacheRecency_.splice(cacheRecency_.end(), cacheRecency_, it->second.recency);
                return it->second.response;
            }
            cacheRecency_.erase(it->second.recency);
            apiCache_.erase(it);
        }
    }

    HttpResponse response = handleHttpRequest(method, target, database_.get());
    if (response.status == 200) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        for (auto it = apiCache_.begin(); it != apiCache_.end();) {
            if (it->second.expiresAt <= now) {
                cacheRecency_.erase(it->second.recency);
                it = apiCache_.erase(it);
            } else {
                ++it;
            }
        }
        if (const auto existing = apiCache_.find(key); existing != apiCache_.end()) {
            cacheRecency_.erase(existing->second.recency);
            apiCache_.erase(existing);
        }
        if (apiCache_.size() >= MAX_API_CACHE_ENTRIES) {
            apiCache_.erase(cacheRecency_.front());
            cacheRecency_.pop_front();
        }
        cacheRecency_.push_back(key);
        apiCache_.emplace(
            key,
            CachedHttpResponse{response, now + API_CACHE_TTL, std::prev(cacheRecency_.end())});
    }

    return response;
}

std::size_t VisualizationService::cachedResponseCount() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    return apiCache_.size();
}

} // namespace keyrecord
