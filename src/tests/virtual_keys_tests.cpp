#include "../platform/virtual_keys.h"

// 锁定虚拟键码数值契约：数据库 vk_code 字段、keyboard_layout.cpp 坐标表、
// key_classification.cpp 分区/左右手逻辑以及前端 keyboard.js 都依赖这些具体数值；
// Linux/macOS 采集后端也据此把原生键码归一化为该数值。一旦漂移即静默错位，
// 因此以 static_assert 固定关键条目，作为跨平台改造的回归护栏。

namespace vk = keyrecord::vk;

static_assert(vk::Back == 0x08, "VK contract: Back");
static_assert(vk::Tab == 0x09, "VK contract: Tab");
static_assert(vk::Return == 0x0D, "VK contract: Return");
static_assert(vk::Shift == 0x10, "VK contract: Shift");
static_assert(vk::Control == 0x11, "VK contract: Control");
static_assert(vk::Menu == 0x12, "VK contract: Menu");
static_assert(vk::Capital == 0x14, "VK contract: Capital");
static_assert(vk::Escape == 0x1B, "VK contract: Escape");
static_assert(vk::Space == 0x20, "VK contract: Space");
static_assert(vk::Left == 0x25, "VK contract: Left");
static_assert(vk::Up == 0x26, "VK contract: Up");
static_assert(vk::Right == 0x27, "VK contract: Right");
static_assert(vk::Down == 0x28, "VK contract: Down");
static_assert(vk::Numpad0 == 0x60, "VK contract: Numpad0");
static_assert(vk::Numpad9 == 0x69, "VK contract: Numpad9");
static_assert(vk::Multiply == 0x6A, "VK contract: Multiply");
static_assert(vk::Divide == 0x6F, "VK contract: Divide");
static_assert(vk::F1 == 0x70, "VK contract: F1");
static_assert(vk::F12 == 0x7B, "VK contract: F12");
static_assert(vk::NumLock == 0x90, "VK contract: NumLock");
static_assert(vk::Scroll == 0x91, "VK contract: Scroll");
static_assert(vk::LShift == 0xA0, "VK contract: LShift");
static_assert(vk::RControl == 0xA3, "VK contract: RControl");
static_assert(vk::RMenu == 0xA5, "VK contract: RMenu");
static_assert(vk::Oem1 == 0xBA, "VK contract: Oem1");
static_assert(vk::OemPlus == 0xBB, "VK contract: OemPlus");
static_assert(vk::OemComma == 0xBC, "VK contract: OemComma");
static_assert(vk::OemPeriod == 0xBE, "VK contract: OemPeriod");
static_assert(vk::Oem3 == 0xC0, "VK contract: Oem3 (backtick, 192 in key_classification)");
static_assert(vk::Oem7 == 0xDE, "VK contract: Oem7");

int main() {
    return 0;
}
