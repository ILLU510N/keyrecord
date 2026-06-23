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
        std::cerr << message << "\n按键缺少布局坐标: " << vkCode << "\n";
        return false;
    }

    const bool matches = std::abs(position->x - expectedX) < 0.001 && std::abs(position->y - expectedY) < 0.001;
    if (!matches) {
        std::cerr << message << "\n期望: (" << expectedX << ", " << expectedY << ")"
                  << "\n实际: (" << position->x << ", " << position->y << ")\n";
    }
    return matches;
}

} // namespace

int main() {
    bool ok = true;

    ok = expectPosition(116, 425, 45, "F5 应映射到功能键区") && ok;
    ok = expectPosition(36, 1055, 105, "Home 应映射到导航键区") && ok;
    ok = expectPosition(39, 1115, 345, "Right 应映射到方向键区") && ok;
    ok = expectPosition(98, 1225, 285, "Numpad2 应映射到数字小键盘区") && ok;
    ok = expect(!keyrecord::getKeyPosition(255).has_value(), "未知 vk_code 不应产生布局坐标") && ok;

    return ok ? 0 : 1;
}
