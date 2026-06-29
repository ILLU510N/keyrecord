#include "visualization_service.h"

#include <chrono>
#include <utility>

namespace keyrecord {
namespace {

constexpr auto API_CACHE_TTL = std::chrono::seconds(2);

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
                return it->second.response;
            }
            apiCache_.erase(it);
        }
    }

    HttpResponse response = handleHttpRequest(method, target, database_.get());
    if (response.status == 200) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        apiCache_[key] = CachedHttpResponse{response, now + API_CACHE_TTL};
    }

    return response;
}

} // namespace keyrecord
