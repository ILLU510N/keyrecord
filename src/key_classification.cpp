#include "key_classification.h"

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
        case 192:
        // 左侧控制与修饰键：Esc Tab Caps 左 Shift/Ctrl/Alt
        case 27: case 9: case 20:
        case 160: case 162: case 164:
        // F1-F6
        case 112: case 113: case 114: case 115: case 116: case 117:
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
        case 189: case 187: case 219: case 221: case 220:
        case 186: case 222: case 188: case 190: case 191:
        // 退格与回车
        case 8: case 13:
        // 右 Shift/Ctrl/Alt
        case 161: case 163: case 165:
        // F7-F12
        case 118: case 119: case 120: case 121: case 122: case 123:
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
    if (inRange(vkCode, 96, 111)) {
        return KeyRegion::Numpad;
    }
    if (inRange(vkCode, 112, 123)) {
        return KeyRegion::Function;
    }
    // 方向键、Home/End/PgUp/PgDn、Ins、Del
    if (inRange(vkCode, 33, 40) || vkCode == 45 || vkCode == 46) {
        return KeyRegion::Navigation;
    }
    // Shift/Ctrl/Alt/Win 以及 Caps/Num/Scroll 锁定键
    if (vkCode == 16 || vkCode == 17 || vkCode == 18 || vkCode == 91 || vkCode == 92 ||
        inRange(vkCode, 160, 165) || vkCode == 20 || vkCode == 144 || vkCode == 145) {
        return KeyRegion::Modifiers;
    }
    // 空格、回车、Tab、退格、Esc
    if (vkCode == 8 || vkCode == 9 || vkCode == 13 || vkCode == 27 || vkCode == 32) {
        return KeyRegion::Control;
    }
    // 标点符号
    if (inRange(vkCode, 186, 192) || inRange(vkCode, 219, 222)) {
        return KeyRegion::Punctuation;
    }
    return KeyRegion::Other;
}

KeyHand getKeyHand(int vkCode) {
    if (vkCode == 32) {
        return KeyHand::Both;
    }
    if (isLeftHandKey(vkCode)) {
        return KeyHand::Left;
    }
    if (isRightHandKey(vkCode)) {
        return KeyHand::Right;
    }
    // 小键盘整体归右手，便于右手数字输入对比。
    if (inRange(vkCode, 96, 111) || vkCode == 144) {
        return KeyHand::Right;
    }
    // 导航与编辑键簇位于键盘右侧，归右手。
    if (inRange(vkCode, 33, 40) || vkCode == 45 || vkCode == 46) {
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
