#pragma once

#include "platform/key_code.h"
#include "platform/virtual_keys.h"

namespace keyrecord {

// macOS 虚拟键码（Carbon <HIToolbox/Events.h> 的 kVK_*）到 Windows 虚拟键码的映射。
// 数据库 vk_code 与前端 keyboard.js 以 Windows VK 数值为契约，macOS 采集后端
// （CGEventTap 的 kCGKeyboardEventKeycode）必须在入库前完成此归一化。
// 返回 0 表示未映射。函数纯数值、零 macOS 头依赖，可在任意平台单元测试
// （见 src/tests/macos_keymap_tests.cpp）。
//
// 归一化约定：Command 归一化为 Win 键、Option 归一化为 Alt（两者语义对应），
// 主键区 Delete(0x33) 为退格，ForwardDelete(0x75) 为删除。
constexpr KeyCode macVirtualKeyToVk(int code) {
    switch (code) {
        // 字母
        case 0x00: return static_cast<KeyCode>('A');
        case 0x0B: return static_cast<KeyCode>('B');
        case 0x08: return static_cast<KeyCode>('C');
        case 0x02: return static_cast<KeyCode>('D');
        case 0x0E: return static_cast<KeyCode>('E');
        case 0x03: return static_cast<KeyCode>('F');
        case 0x05: return static_cast<KeyCode>('G');
        case 0x04: return static_cast<KeyCode>('H');
        case 0x22: return static_cast<KeyCode>('I');
        case 0x26: return static_cast<KeyCode>('J');
        case 0x28: return static_cast<KeyCode>('K');
        case 0x25: return static_cast<KeyCode>('L');
        case 0x2E: return static_cast<KeyCode>('M');
        case 0x2D: return static_cast<KeyCode>('N');
        case 0x1F: return static_cast<KeyCode>('O');
        case 0x23: return static_cast<KeyCode>('P');
        case 0x0C: return static_cast<KeyCode>('Q');
        case 0x0F: return static_cast<KeyCode>('R');
        case 0x01: return static_cast<KeyCode>('S');
        case 0x11: return static_cast<KeyCode>('T');
        case 0x20: return static_cast<KeyCode>('U');
        case 0x09: return static_cast<KeyCode>('V');
        case 0x0D: return static_cast<KeyCode>('W');
        case 0x07: return static_cast<KeyCode>('X');
        case 0x10: return static_cast<KeyCode>('Y');
        case 0x06: return static_cast<KeyCode>('Z');

        // 主键盘数字区
        case 0x12: return static_cast<KeyCode>('1');
        case 0x13: return static_cast<KeyCode>('2');
        case 0x14: return static_cast<KeyCode>('3');
        case 0x15: return static_cast<KeyCode>('4');
        case 0x17: return static_cast<KeyCode>('5');
        case 0x16: return static_cast<KeyCode>('6');
        case 0x1A: return static_cast<KeyCode>('7');
        case 0x1C: return static_cast<KeyCode>('8');
        case 0x19: return static_cast<KeyCode>('9');
        case 0x1D: return static_cast<KeyCode>('0');

        // 标点与符号
        case 0x1B: return vk::OemMinus;   // Minus  -
        case 0x18: return vk::OemPlus;    // Equal  =
        case 0x21: return vk::Oem4;       // LeftBracket  [
        case 0x1E: return vk::Oem6;       // RightBracket ]
        case 0x29: return vk::Oem1;       // Semicolon ;
        case 0x27: return vk::Oem7;       // Quote '
        case 0x32: return vk::Oem3;       // Grave `
        case 0x2A: return vk::Oem5;       // Backslash \.
        case 0x2B: return vk::OemComma;   // Comma ,
        case 0x2F: return vk::OemPeriod;  // Period .
        case 0x2C: return vk::Oem2;       // Slash /

        // 控制键
        case 0x24: return vk::Return;     // Return
        case 0x30: return vk::Tab;        // Tab
        case 0x31: return vk::Space;      // Space
        case 0x33: return vk::Back;       // Delete (退格)
        case 0x35: return vk::Escape;     // Escape

        // 修饰键（Command->Win, Option->Alt）
        case 0x37: return vk::LWin;       // Command
        case 0x36: return vk::RWin;       // RightCommand
        case 0x38: return vk::LShift;     // Shift
        case 0x3C: return vk::RShift;     // RightShift
        case 0x3A: return vk::LMenu;      // Option
        case 0x3D: return vk::RMenu;      // RightOption
        case 0x3B: return vk::LControl;   // Control
        case 0x3E: return vk::RControl;   // RightControl
        case 0x39: return vk::Capital;    // CapsLock

        // 小键盘
        case 0x41: return vk::Decimal;    // KeypadDecimal
        case 0x43: return vk::Multiply;   // KeypadMultiply
        case 0x45: return vk::Add;        // KeypadPlus
        case 0x47: return vk::NumLock;    // KeypadClear ~ NumLock
        case 0x4B: return vk::Divide;     // KeypadDivide
        case 0x4C: return vk::Return;     // KeypadEnter -> Enter
        case 0x4E: return vk::Subtract;   // KeypadMinus
        case 0x52: return vk::Numpad0;
        case 0x53: return vk::Numpad1;
        case 0x54: return vk::Numpad2;
        case 0x55: return vk::Numpad3;
        case 0x56: return vk::Numpad4;
        case 0x57: return vk::Numpad5;
        case 0x58: return vk::Numpad6;
        case 0x59: return vk::Numpad7;
        case 0x5B: return vk::Numpad8;
        case 0x5C: return vk::Numpad9;

        // 功能键（macOS 键码不连续）
        case 0x7A: return vk::F1;
        case 0x78: return vk::F2;
        case 0x63: return vk::F3;
        case 0x76: return vk::F4;
        case 0x60: return vk::F5;
        case 0x61: return vk::F6;
        case 0x62: return vk::F7;
        case 0x64: return vk::F8;
        case 0x65: return vk::F9;
        case 0x6D: return vk::F10;
        case 0x67: return vk::F11;
        case 0x6F: return vk::F12;

        // 导航与编辑键簇
        case 0x73: return vk::Home;       // Home
        case 0x77: return vk::End;        // End
        case 0x74: return vk::Prior;      // PageUp
        case 0x79: return vk::Next;       // PageDown
        case 0x75: return vk::Delete;     // ForwardDelete
        case 0x7B: return vk::Left;       // LeftArrow
        case 0x7C: return vk::Right;      // RightArrow
        case 0x7D: return vk::Down;       // DownArrow
        case 0x7E: return vk::Up;         // UpArrow

        default: return 0;
    }
}

} // namespace keyrecord
