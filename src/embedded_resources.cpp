#include "embedded_resources.h"

#include "generated_resources.h"

#include <algorithm>

namespace keyrecord {
namespace {

std::string_view extensionOf(std::string_view path) {
    const auto slash = path.find_last_of('/');
    const auto dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return {};
    }
    return path.substr(dot);
}

} // namespace

std::string normalizeResourcePath(std::string_view requestPath) {
    const auto queryPos = requestPath.find_first_of("?#");
    if (queryPos != std::string_view::npos) {
        requestPath = requestPath.substr(0, queryPos);
    }

    if (requestPath.empty() || requestPath == "/") {
        return "/index.html";
    }

    if (requestPath.find('\\') != std::string_view::npos) {
        return {};
    }

    std::string normalized;
    normalized.push_back('/');
    std::size_t offset = requestPath.front() == '/' ? 1 : 0;
    while (offset <= requestPath.size()) {
        const auto slash = requestPath.find('/', offset);
        const auto segment = requestPath.substr(offset, slash - offset);
        if (segment == "..") {
            return {};
        }
        if (!segment.empty() && segment != ".") {
            if (normalized.size() > 1) {
                normalized.push_back('/');
            }
            normalized.append(segment);
        }
        if (slash == std::string_view::npos) {
            break;
        }
        offset = slash + 1;
    }
    return normalized == "/" ? "/index.html" : normalized;
}

std::string_view contentTypeForPath(std::string_view path) {
    const auto extension = extensionOf(path);
    if (extension == ".html" || extension == ".htm") {
        return "text/html; charset=utf-8";
    }
    if (extension == ".css") {
        return "text/css; charset=utf-8";
    }
    if (extension == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (extension == ".json") {
        return "application/json; charset=utf-8";
    }
    if (extension == ".svg") {
        return "image/svg+xml";
    }
    if (extension == ".png") {
        return "image/png";
    }
    if (extension == ".ico") {
        return "image/x-icon";
    }
    return "application/octet-stream";
}

bool embeddedResourceIndexIsSorted() {
    const auto resources = generatedEmbeddedResources();
    return std::is_sorted(resources.begin(), resources.end(), [](const EmbeddedResourceEntry& left,
                                                                 const EmbeddedResourceEntry& right) {
        return left.path < right.path;
    });
}

std::optional<EmbeddedResource> findEmbeddedResource(std::string_view requestPath) {
    const std::string normalizedPath = normalizeResourcePath(requestPath);
    if (normalizedPath.empty()) {
        return std::nullopt;
    }

    const auto resources = generatedEmbeddedResources();
    const auto it = std::lower_bound(resources.begin(),
                                     resources.end(),
                                     std::string_view(normalizedPath),
                                     [](const EmbeddedResourceEntry& entry, std::string_view path) {
                                         return entry.path < path;
                                     });

    if (it == resources.end() || it->path != normalizedPath) {
        return std::nullopt;
    }

    return EmbeddedResource{
        it->path,
        contentTypeForPath(it->path),
        std::span<const unsigned char>(it->data, it->size),
    };
}

} // namespace keyrecord
