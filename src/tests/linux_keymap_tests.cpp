#include "../platform/linux_keymap.h"
#include "../platform/virtual_keys.h"

// Linux evdev 键码归一化的回归护栏。映射为纯数值函数，可在任意平台编译执行，
// 因此无需真实 Linux 环境即可锁定契约的关键条目。

namespace vk = keyrecord::vk;
using keyrecord::linuxEvdevToVk;

// 字母 / 数字直接映射到 ASCII 大写与数字字符。
static_assert(linuxEvdevToVk(30) == static_cast<keyrecord::KeyCode>('A'), "KEY_A -> 'A'");
static_assert(linuxEvdevToVk(50) == static_cast<keyrecord::KeyCode>('M'), "KEY_M -> 'M'");
static_assert(linuxEvdevToVk(16) == static_cast<keyrecord::KeyCode>('Q'), "KEY_Q -> 'Q'");
static_assert(linuxEvdevToVk(2) == static_cast<keyrecord::KeyCode>('1'), "KEY_1 -> '1'");
static_assert(linuxEvdevToVk(11) == static_cast<keyrecord::KeyCode>('0'), "KEY_0 -> '0'");

// 控制 / 修饰 / 符号 / 导航 / 小键盘代表性条目。
static_assert(linuxEvdevToVk(1) == vk::Escape, "KEY_ESC -> Escape");
static_assert(linuxEvdevToVk(28) == vk::Return, "KEY_ENTER -> Return");
static_assert(linuxEvdevToVk(96) == vk::Return, "KEY_KPENTER -> Return");
static_assert(linuxEvdevToVk(57) == vk::Space, "KEY_SPACE -> Space");
static_assert(linuxEvdevToVk(14) == vk::Back, "KEY_BACKSPACE -> Back");
static_assert(linuxEvdevToVk(41) == vk::Oem3, "KEY_GRAVE -> Oem3 (backtick)");
static_assert(linuxEvdevToVk(40) == vk::Oem7, "KEY_APOSTROPHE -> Oem7");
static_assert(linuxEvdevToVk(42) == vk::LShift, "KEY_LEFTSHIFT -> LShift");
static_assert(linuxEvdevToVk(54) == vk::RShift, "KEY_RIGHTSHIFT -> RShift");
static_assert(linuxEvdevToVk(29) == vk::LControl, "KEY_LEFTCTRL -> LControl");
static_assert(linuxEvdevToVk(59) == vk::F1, "KEY_F1 -> F1");
static_assert(linuxEvdevToVk(88) == vk::F12, "KEY_F12 -> F12");
static_assert(linuxEvdevToVk(82) == vk::Numpad0, "KEY_KP0 -> Numpad0");
static_assert(linuxEvdevToVk(98) == vk::Divide, "KEY_KPSLASH -> Divide");
static_assert(linuxEvdevToVk(103) == vk::Up, "KEY_UP -> Up");
static_assert(linuxEvdevToVk(111) == vk::Delete, "KEY_DELETE -> Delete");

// 未映射键码返回 0。
static_assert(linuxEvdevToVk(0) == 0, "KEY_RESERVED -> 0");
static_assert(linuxEvdevToVk(9999) == 0, "out-of-range -> 0");

int main() {
    return 0;
}
