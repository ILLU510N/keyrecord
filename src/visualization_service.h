#pragma once

#include "http_router.h"
#include "readonly_database.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <list>
#include <unordered_map>

namespace keyrecord {

class VisualizationService {
public:
    static std::optional<VisualizationService> open(const std::string& dbPath, std::string* errorMessage = nullptr);

    VisualizationService(const VisualizationService&) = delete;
    VisualizationService& operator=(const VisualizationService&) = delete;
    VisualizationService(VisualizationService&& other) noexcept;
    VisualizationService& operator=(VisualizationService&& other) noexcept;
    ~VisualizationService() = default;

    HttpResponse handleRequest(std::string_view method, std::string_view target) const;

    // 仅供测试和运行状态观测，避免缓存增长问题只能通过内存侧面推断。
    std::size_t cachedResponseCount() const;

private:
    struct CachedHttpResponse {
        HttpResponse response;
        std::chrono::steady_clock::time_point expiresAt;
        std::list<std::string>::iterator recency;
    };

    explicit VisualizationService(ReadOnlyDatabase database);

    ReadOnlyDatabase database_;
    mutable std::mutex cacheMutex_;
    mutable std::list<std::string> cacheRecency_;
    mutable std::unordered_map<std::string, CachedHttpResponse> apiCache_;
};

} // namespace keyrecord
