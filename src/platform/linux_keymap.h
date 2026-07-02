#pragma once

#include "platform/key_code.h"
#include "platform/virtual_keys.h"

namespace keyrecord {

// Linux evdev 键码（linux/input-event-codes.h 中的 KEY_*）到 Windows 虚拟键码的映射。
// 数据库 vk_code 与前端 keyboard.js 以 Windows VK 数值为契约，Linux 采集后端必须在
// 入库前完成此归一化。返回 0 表示未映射（调用方应按未知处理或丢弃）。
//
// 本函数不依赖任何 Linux 头文件，仅做纯数值映射，因此可在任意平台参与单元测试
// （见 src/tests/linux_keymap_tests.cpp），作为跨平台键码契约的回归护栏。
constexpr KeyCode linuxEvdevToVk(int code) {
    switch (code) {
        // 主键盘数字区（KEY_1..KEY_0 = 2..11）
        case 2: return static_cast<KeyCode>('1');
        case 3: return static_cast<KeyCode>('2');
        case 4: return static_cast<KeyCode>('3');
        case 5: return static_cast<KeyCode>('4');
        case 6: return static_cast<KeyCode>('5');
        case 7: return static_cast<KeyCode>('6');
        case 8: return static_cast<KeyCode>('7');
        case 9: return static_cast<KeyCode>('8');
        case 10: return static_cast<KeyCode>('9');
        case 11: return static_cast<KeyCode>('0');

        // 字母区（按 evdev 物理顺序）
        case 16: return static_cast<KeyCode>('Q');
        case 17: return static_cast<KeyCode>('W');
        case 18: return static_cast<KeyCode>('E');
        case 19: return static_cast<KeyCode>('R');
        case 20: return static_cast<KeyCode>('T');
        case 21: return static_cast<KeyCode>('Y');
        case 22: return static_cast<KeyCode>('U');
        case 23: return static_cast<KeyCode>('I');
        case 24: return static_cast<KeyCode>('O');
        case 25: return static_cast<KeyCode>('P');
        case 30: return static_cast<KeyCode>('A');
        case 31: return static_cast<KeyCode>('S');
        case 32: return static_cast<KeyCode>('D');
        case 33: return static_cast<KeyCode>('F');
        case 34: return static_cast<KeyCode>('G');
        case 35: return static_cast<KeyCode>('H');
        case 36: return static_cast<KeyCode>('J');
        case 37: return static_cast<KeyCode>('K');
        case 38: return static_cast<KeyCode>('L');
        case 44: return static_cast<KeyCode>('Z');
        case 45: return static_cast<KeyCode>('X');
        case 46: return static_cast<KeyCode>('C');
        case 47: return static_cast<KeyCode>('V');
        case 48: return static_cast<KeyCode>('B');
        case 49: return static_cast<KeyCode>('N');
        case 50: return static_cast<KeyCode>('M');

        // 控制与编辑键
        case 1: return vk::Escape;      // KEY_ESC
        case 14: return vk::Back;       // KEY_BACKSPACE
        case 15: return vk::Tab;        // KEY_TAB
        case 28: return vk::Return;     // KEY_ENTER
        case 57: return vk::Space;      // KEY_SPACE
        case 58: return vk::Capital;    // KEY_CAPSLOCK
        case 119: return vk::Pause;     // KEY_PAUSE

        // 标点与符号
        case 12: return vk::OemMinus;   // KEY_MINUS
        case 13: return vk::OemPlus;    // KEY_EQUAL
        case 26: return vk::Oem4;       // KEY_LEFTBRACE  [
        case 27: return vk::Oem6;       // KEY_RIGHTBRACE ]
        case 39: return vk::Oem1;       // KEY_SEMICOLON  ;
        case 40: return vk::Oem7;       // KEY_APOSTROPHE '
        case 41: return vk::Oem3;       // KEY_GRAVE      `
        case 43: return vk::Oem5;       // KEY_BACKSLASH  \.
        case 51: return vk::OemComma;   // KEY_COMMA      ,
        case 52: return vk::OemPeriod;  // KEY_DOT        .
        case 53: return vk::Oem2;       // KEY_SLASH      /

        // 修饰键（区分左右）
        case 29: return vk::LControl;   // KEY_LEFTCTRL
        case 42: return vk::LShift;     // KEY_LEFTSHIFT
        case 54: return vk::RShift;     // KEY_RIGHTSHIFT
        case 56: return vk::LMenu;      // KEY_LEFTALT
        case 97: return vk::RControl;   // KEY_RIGHTCTRL
        case 100: return vk::RMenu;     // KEY_RIGHTALT
        case 125: return vk::LWin;      // KEY_LEFTMETA
        case 126: return vk::RWin;      // KEY_RIGHTMETA
        case 127: return vk::Apps;      // KEY_COMPOSE (menu)

        // 功能键 F1-F12（注意 F11/F12 的 evdev 码不连续）
        case 59: return vk::F1;
        case 60: return vk::F2;
        case 61: return vk::F3;
        case 62: return vk::F4;
        case 63: return vk::F5;
        case 64: return vk::F6;
        case 65: return vk::F7;
        case 66: return vk::F8;
        case 67: return vk::F9;
        case 68: return vk::F10;
        case 87: return vk::F11;
        case 88: return vk::F12;

        // 锁定键
        case 69: return vk::NumLock;    // KEY_NUMLOCK
        case 70: return vk::Scroll;     // KEY_SCROLLLOCK

        // 小键盘
        case 55: return vk::Multiply;   // KEY_KPASTERISK
        case 71: return vk::Numpad7;
        case 72: return vk::Numpad8;
        case 73: return vk::Numpad9;
        case 74: return vk::Subtract;   // KEY_KPMINUS
        case 75: return vk::Numpad4;
        case 76: return vk::Numpad5;
        case 77: return vk::Numpad6;
        case 78: return vk::Add;        // KEY_KPPLUS
        case 79: return vk::Numpad1;
        case 80: return vk::Numpad2;
        case 81: return vk::Numpad3;
        case 82: return vk::Numpad0;
        case 83: return vk::Decimal;    // KEY_KPDOT
        case 96: return vk::Return;     // KEY_KPENTER -> Enter
        case 98: return vk::Divide;     // KEY_KPSLASH

        // 导航与编辑键簇
        case 99: return vk::Snapshot;   // KEY_SYSRQ (PrintScreen)
        case 102: return vk::Home;      // KEY_HOME
        case 103: return vk::Up;        // KEY_UP
        case 104: return vk::Prior;     // KEY_PAGEUP
        case 105: return vk::Left;      // KEY_LEFT
        case 106: return vk::Right;     // KEY_RIGHT
        case 107: return vk::End;       // KEY_END
        case 108: return vk::Down;      // KEY_DOWN
        case 109: return vk::Next;      // KEY_PAGEDOWN
        case 110: return vk::Insert;    // KEY_INSERT
        case 111: return vk::Delete;    // KEY_DELETE

        default: return 0;
    }
}

} // namespace keyrecord
