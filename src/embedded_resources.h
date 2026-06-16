#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace keyrecord {

struct EmbeddedResource {
    std::string_view path;
    std::string_view contentType;
    std::span<const unsigned char> data;
};

std::string normalizeResourcePath(std::string_view requestPath);
std::string_view contentTypeForPath(std::string_view path);
bool embeddedResourceIndexIsSorted();
std::optional<EmbeddedResource> findEmbeddedResource(std::string_view requestPath);

} // namespace keyrecord
