#include "visualization_service.h"

#include <utility>

namespace keyrecord {

VisualizationService::VisualizationService(ReadOnlyDatabase database)
    : database_(std::move(database)) {
}

std::optional<VisualizationService> VisualizationService::open(const std::string& dbPath, std::string* errorMessage) {
    auto database = ReadOnlyDatabase::open(dbPath, errorMessage);
    if (!database) {
        return std::nullopt;
    }

    return VisualizationService(std::move(*database));
}

HttpResponse VisualizationService::handleRequest(std::string_view method, std::string_view target) const {
    return handleHttpRequest(method, target, database_.get());
}

} // namespace keyrecord
