#include "key_classification.h"

#include "platform/virtual_keys.h"

namespace keyrecord {
namespace {

bool inRange(int value, int low, int high) {
    return value >= low && value <= high;
}

// 标准盲打指法下归属左手的按键虚拟码集合。
bool isLeftHandKey(int vkCode) {
    switch (vkCode) {
        // 字母区左手列
        case 'Q': case 'W': case 'E': case 'R': case 'T':
        case 'A': case 'S': case 'D': case 'F': case 'G':
        case 'Z': case 'X': case 'C': case 'V': case 'B':
        // 数字区左手列 1-5 与反引号
        case '1': case '2': case '3': case '4': case '5':
        case vk::Oem3:
        // 左侧控制与修饰键：Esc Tab Caps 左 Shift/Ctrl/Alt
        case vk::Escape: case vk::Tab: case vk::Capital:
        case vk::LShift: case vk::LControl: case vk::LMenu:
        // F1-F6
        case vk::F1: case vk::F2: case vk::F3: case vk::F4: case vk::F5: case vk::F6:
            return true;
        default:
            return false;
    }
}

// 标准盲打指法下归属右手的按键虚拟码集合。
bool isRightHandKey(int vkCode) {
    switch (vkCode) {
        // 字母区右手列
        case 'Y': case 'U': case 'I': case 'O': case 'P':
        case 'H': case 'J': case 'K': case 'L':
        case 'N': case 'M':
        // 数字区右手列 6-0
        case '6': case '7': case '8': case '9': case '0':
        // 右侧符号
        case vk::OemMinus: case vk::OemPlus: case vk::Oem4: case vk::Oem6: case vk::Oem5:
        case vk::Oem1: case vk::Oem7: case vk::OemComma: case vk::OemPeriod: case vk::Oem2:
        // 退格与回车
        case vk::Back: case vk::Return:
        // 右 Shift/Ctrl/Alt
        case vk::RShift: case vk::RControl: case vk::RMenu:
        // F7-F12
        case vk::F7: case vk::F8: case vk::F9: case vk::F10: case vk::F11: case vk::F12:
            return true;
        default:
            return false;
    }
}

} // namespace

KeyRegion getKeyRegion(int vkCode) {
    if (inRange(vkCode, 'A', 'Z')) {
        return KeyRegion::Letters;
    }
    if (inRange(vkCode, '0', '9')) {
        return KeyRegion::Digits;
    }
    if (inRange(vkCode, vk::Numpad0, vk::Divide)) {
        return KeyRegion::Numpad;
    }
    if (inRange(vkCode, vk::F1, vk::F12)) {
        return KeyRegion::Function;
    }
    // 方向键、Home/End/PgUp/PgDn、Ins、Del
    if (inRange(vkCode, vk::Prior, vk::Down) || vkCode == vk::Insert || vkCode == vk::Delete) {
        return KeyRegion::Navigation;
    }
    // Shift/Ctrl/Alt/Win 以及 Caps/Num/Scroll 锁定键
    if (vkCode == vk::Shift || vkCode == vk::Control || vkCode == vk::Menu ||
        vkCode == vk::LWin || vkCode == vk::RWin || inRange(vkCode, vk::LShift, vk::RMenu) ||
        vkCode == vk::Capital || vkCode == vk::NumLock || vkCode == vk::Scroll) {
        return KeyRegion::Modifiers;
    }
    // 空格、回车、Tab、退格、Esc
    if (vkCode == vk::Back || vkCode == vk::Tab || vkCode == vk::Return ||
        vkCode == vk::Escape || vkCode == vk::Space) {
        return KeyRegion::Control;
    }
    // 标点符号
    if (inRange(vkCode, vk::Oem1, vk::Oem3) || inRange(vkCode, vk::Oem4, vk::Oem7)) {
        return KeyRegion::Punctuation;
    }
    return KeyRegion::Other;
}

KeyHand getKeyHand(int vkCode) {
    if (vkCode == vk::Space) {
        return KeyHand::Both;
    }
    if (isLeftHandKey(vkCode)) {
        return KeyHand::Left;
    }
    if (isRightHandKey(vkCode)) {
        return KeyHand::Right;
    }
    // 小键盘整体归右手，便于右手数字输入对比。
    if (inRange(vkCode, vk::Numpad0, vk::Divide) || vkCode == vk::NumLock) {
        return KeyHand::Right;
    }
    // 导航与编辑键簇位于键盘右侧，归右手。
    if (inRange(vkCode, vk::Prior, vk::Down) || vkCode == vk::Insert || vkCode == vk::Delete) {
        return KeyHand::Right;
    }
    return KeyHand::Unknown;
}

std::string_view regionName(KeyRegion region) {
    switch (region) {
        case KeyRegion::Letters:
            return "letters";
        case KeyRegion::Digits:
            return "digits";
        case KeyRegion::Numpad:
            return "numpad";
        case KeyRegion::Function:
            return "function";
        case KeyRegion::Navigation:
            return "navigation";
        case KeyRegion::Modifiers:
            return "modifiers";
        case KeyRegion::Control:
            return "control";
        case KeyRegion::Punctuation:
            return "punctuation";
        case KeyRegion::Other:
            return "other";
    }
    return "other";
}

std::string_view handName(KeyHand hand) {
    switch (hand) {
        case KeyHand::Left:
            return "left";
        case KeyHand::Right:
            return "right";
        case KeyHand::Both:
            return "both";
        case KeyHand::Unknown:
            return "unknown";
    }
    return "unknown";
}

} // namespace keyrecord
