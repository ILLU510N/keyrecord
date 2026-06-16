#pragma once

#include "http_router.h"
#include "readonly_database.h"

#include <optional>
#include <string>
#include <string_view>

namespace keyrecord {

class VisualizationService {
public:
    static std::optional<VisualizationService> open(const std::string& dbPath, std::string* errorMessage = nullptr);

    VisualizationService(const VisualizationService&) = delete;
    VisualizationService& operator=(const VisualizationService&) = delete;
    VisualizationService(VisualizationService&&) noexcept = default;
    VisualizationService& operator=(VisualizationService&&) noexcept = default;
    ~VisualizationService() = default;

    HttpResponse handleRequest(std::string_view method, std::string_view target) const;

private:
    explicit VisualizationService(ReadOnlyDatabase database);

    ReadOnlyDatabase database_;
};

} // namespace keyrecord
