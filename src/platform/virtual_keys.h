#pragma once

#include "platform/key_code.h"

namespace keyrecord::vk {

// 可移植的 Windows 虚拟键码常量，数值与 <windows.h> 中的 VK_* 完全一致。
// 数据库 vk_code 字段、keyboard_layout.cpp 坐标表、key_classification.cpp 分区
// 逻辑以及前端 keyboard.js 都以这些数值为契约，因此这里是三平台采集后端
// 归一化键码时共同引用的唯一事实源。使用命名空间常量而非宏，避免与实际
// 包含 <windows.h> 的翻译单元发生宏冲突。

inline constexpr KeyCode Back = 0x08;
inline constexpr KeyCode Tab = 0x09;
inline constexpr KeyCode Return = 0x0D;
inline constexpr KeyCode Shift = 0x10;
inline constexpr KeyCode Control = 0x11;
inline constexpr KeyCode Menu = 0x12;
inline constexpr KeyCode Pause = 0x13;
inline constexpr KeyCode Capital = 0x14;
inline constexpr KeyCode Escape = 0x1B;
inline constexpr KeyCode Space = 0x20;
inline constexpr KeyCode Prior = 0x21;
inline constexpr KeyCode Next = 0x22;
inline constexpr KeyCode End = 0x23;
inline constexpr KeyCode Home = 0x24;
inline constexpr KeyCode Left = 0x25;
inline constexpr KeyCode Up = 0x26;
inline constexpr KeyCode Right = 0x27;
inline constexpr KeyCode Down = 0x28;
inline constexpr KeyCode Snapshot = 0x2C;
inline constexpr KeyCode Insert = 0x2D;
inline constexpr KeyCode Delete = 0x2E;
inline constexpr KeyCode LWin = 0x5B;
inline constexpr KeyCode RWin = 0x5C;
inline constexpr KeyCode Apps = 0x5D;
inline constexpr KeyCode Numpad0 = 0x60;
inline constexpr KeyCode Numpad1 = 0x61;
inline constexpr KeyCode Numpad2 = 0x62;
inline constexpr KeyCode Numpad3 = 0x63;
inline constexpr KeyCode Numpad4 = 0x64;
inline constexpr KeyCode Numpad5 = 0x65;
inline constexpr KeyCode Numpad6 = 0x66;
inline constexpr KeyCode Numpad7 = 0x67;
inline constexpr KeyCode Numpad8 = 0x68;
inline constexpr KeyCode Numpad9 = 0x69;
inline constexpr KeyCode Multiply = 0x6A;
inline constexpr KeyCode Add = 0x6B;
inline constexpr KeyCode Subtract = 0x6D;
inline constexpr KeyCode Decimal = 0x6E;
inline constexpr KeyCode Divide = 0x6F;
inline constexpr KeyCode F1 = 0x70;
inline constexpr KeyCode F2 = 0x71;
inline constexpr KeyCode F3 = 0x72;
inline constexpr KeyCode F4 = 0x73;
inline constexpr KeyCode F5 = 0x74;
inline constexpr KeyCode F6 = 0x75;
inline constexpr KeyCode F7 = 0x76;
inline constexpr KeyCode F8 = 0x77;
inline constexpr KeyCode F9 = 0x78;
inline constexpr KeyCode F10 = 0x79;
inline constexpr KeyCode F11 = 0x7A;
inline constexpr KeyCode F12 = 0x7B;
inline constexpr KeyCode NumLock = 0x90;
inline constexpr KeyCode Scroll = 0x91;
inline constexpr KeyCode LShift = 0xA0;
inline constexpr KeyCode RShift = 0xA1;
inline constexpr KeyCode LControl = 0xA2;
inline constexpr KeyCode RControl = 0xA3;
inline constexpr KeyCode LMenu = 0xA4;
inline constexpr KeyCode RMenu = 0xA5;
inline constexpr KeyCode Oem1 = 0xBA;
inline constexpr KeyCode OemPlus = 0xBB;
inline constexpr KeyCode OemComma = 0xBC;
inline constexpr KeyCode OemMinus = 0xBD;
inline constexpr KeyCode OemPeriod = 0xBE;
inline constexpr KeyCode Oem2 = 0xBF;
inline constexpr KeyCode Oem3 = 0xC0;
inline constexpr KeyCode Oem4 = 0xDB;
inline constexpr KeyCode Oem5 = 0xDC;
inline constexpr KeyCode Oem6 = 0xDD;
inline constexpr KeyCode Oem7 = 0xDE;

} // namespace keyrecord::vk
