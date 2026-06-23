#pragma once

#include <string_view>

namespace keyrecord {

// 键盘分区：用于"键盘分区统计"。
enum class KeyRegion {
    Letters,     // 字母区 A-Z
    Digits,      // 主键盘数字区 0-9
    Numpad,      // 小键盘
    Function,    // 功能键 F1-F12
    Navigation,  // 方向键与编辑导航键
    Modifiers,   // 修饰键与锁定键
    Control,     // 空格、回车、Tab、退格、Esc 等控制键
    Punctuation, // 标点符号
    Other,       // 其它未归类按键
};

// 左右手归属：用于"左右手使用频率对比"，基于标准盲打指法。
enum class KeyHand {
    Left,
    Right,
    Both, // 拇指键（空格）
    Unknown,
};

KeyRegion getKeyRegion(int vkCode);
KeyHand getKeyHand(int vkCode);

std::string_view regionName(KeyRegion region);
std::string_view handName(KeyHand hand);

} // namespace keyrecord
