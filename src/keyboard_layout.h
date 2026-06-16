#pragma once

#include <optional>

namespace keyrecord {

struct KeyPosition {
    double x;
    double y;
    int width;
    int height;
};

std::optional<KeyPosition> getKeyPosition(int vkCode);

} // namespace keyrecord
