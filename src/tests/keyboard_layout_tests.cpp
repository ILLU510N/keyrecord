#include "../keyboard_layout.h"

#include <cmath>
#include <iostream>
#include <optional>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectPosition(int vkCode, double expectedX, double expectedY, const char* message) {
    const std::optional<keyrecord::KeyPosition> position = keyrecord::getKeyPosition(vkCode);
    if (!position) {
        std::cerr << message << "\nMissing layout coordinates for key: " << vkCode << "\n";
        return false;
    }

    const bool matches = std::abs(position->x - expectedX) < 0.001 && std::abs(position->y - expectedY) < 0.001;
    if (!matches) {
        std::cerr << message << "\nExpected: (" << expectedX << ", " << expectedY << ")"
                  << "\nActual: (" << position->x << ", " << position->y << ")\n";
    }
    return matches;
}

} // namespace

int main() {
    bool ok = true;

    ok = expectPosition(116, 425, 45, "F5 should map to the function key area") && ok;
    ok = expectPosition(36, 1055, 105, "Home should map to the navigation key area") && ok;
    ok = expectPosition(39, 1115, 345, "Right should map to the arrow key area") && ok;
    ok = expectPosition(98, 1225, 285, "Numpad2 should map to the numpad area") && ok;
    ok = expect(!keyrecord::getKeyPosition(255).has_value(), "Unknown vk_code should not produce layout coordinates") && ok;

    return ok ? 0 : 1;
}
