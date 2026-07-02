#include "../platform/macos_keymap.h"
#include "../platform/virtual_keys.h"

// macOS 虚拟键码归一化的回归护栏。映射为纯数值函数，可在任意平台编译执行，
// 因此无需真实 macOS 环境即可锁定契约的关键条目。

namespace vk = keyrecord::vk;
using keyrecord::macVirtualKeyToVk;

// 字母 / 数字映射到 ASCII 大写与数字字符（注意 macOS 键码非物理顺序）。
static_assert(macVirtualKeyToVk(0x00) == static_cast<keyrecord::KeyCode>('A'), "kVK_ANSI_A -> 'A'");
static_assert(macVirtualKeyToVk(0x06) == static_cast<keyrecord::KeyCode>('Z'), "kVK_ANSI_Z -> 'Z'");
static_assert(macVirtualKeyToVk(0x0C) == static_cast<keyrecord::KeyCode>('Q'), "kVK_ANSI_Q -> 'Q'");
static_assert(macVirtualKeyToVk(0x12) == static_cast<keyrecord::KeyCode>('1'), "kVK_ANSI_1 -> '1'");
static_assert(macVirtualKeyToVk(0x1D) == static_cast<keyrecord::KeyCode>('0'), "kVK_ANSI_0 -> '0'");

// 控制 / 修饰 / 符号 / 导航 / 小键盘 / 功能键代表性条目。
static_assert(macVirtualKeyToVk(0x35) == vk::Escape, "kVK_Escape -> Escape");
static_assert(macVirtualKeyToVk(0x24) == vk::Return, "kVK_Return -> Return");
static_assert(macVirtualKeyToVk(0x4C) == vk::Return, "kVK_ANSI_KeypadEnter -> Return");
static_assert(macVirtualKeyToVk(0x31) == vk::Space, "kVK_Space -> Space");
static_assert(macVirtualKeyToVk(0x33) == vk::Back, "kVK_Delete -> Back (backspace)");
static_assert(macVirtualKeyToVk(0x75) == vk::Delete, "kVK_ForwardDelete -> Delete");
static_assert(macVirtualKeyToVk(0x32) == vk::Oem3, "kVK_ANSI_Grave -> Oem3 (backtick)");
static_assert(macVirtualKeyToVk(0x27) == vk::Oem7, "kVK_ANSI_Quote -> Oem7");
static_assert(macVirtualKeyToVk(0x38) == vk::LShift, "kVK_Shift -> LShift");
static_assert(macVirtualKeyToVk(0x3C) == vk::RShift, "kVK_RightShift -> RShift");
static_assert(macVirtualKeyToVk(0x37) == vk::LWin, "kVK_Command -> LWin");
static_assert(macVirtualKeyToVk(0x3A) == vk::LMenu, "kVK_Option -> LMenu (Alt)");
static_assert(macVirtualKeyToVk(0x7A) == vk::F1, "kVK_F1 -> F1");
static_assert(macVirtualKeyToVk(0x6F) == vk::F12, "kVK_F12 -> F12");
static_assert(macVirtualKeyToVk(0x52) == vk::Numpad0, "kVK_ANSI_Keypad0 -> Numpad0");
static_assert(macVirtualKeyToVk(0x4B) == vk::Divide, "kVK_ANSI_KeypadDivide -> Divide");
static_assert(macVirtualKeyToVk(0x7E) == vk::Up, "kVK_UpArrow -> Up");
static_assert(macVirtualKeyToVk(0x73) == vk::Home, "kVK_Home -> Home");

// 未映射键码返回 0。
static_assert(macVirtualKeyToVk(0x3F) == 0, "kVK_Function -> 0 (no VK equivalent)");
static_assert(macVirtualKeyToVk(0x0A) == 0, "unassigned -> 0");

int main() {
    return 0;
}
