#include "key_names.h"

#include "platform/virtual_keys.h"

#include <map>

namespace {

std::map<keyrecord::KeyCode, std::string> vkCodeToName;

} // namespace

void initKeyMap() {
    namespace vk = keyrecord::vk;
    vkCodeToName[vk::Back] = "Backspace";
    vkCodeToName[vk::Tab] = "Tab";
    vkCodeToName[vk::Return] = "Enter";
    vkCodeToName[vk::Shift] = "Shift";
    vkCodeToName[vk::Control] = "Ctrl";
    vkCodeToName[vk::Menu] = "Alt";
    vkCodeToName[vk::Pause] = "Pause";
    vkCodeToName[vk::Capital] = "CapsLock";
    vkCodeToName[vk::Escape] = "Esc";
    vkCodeToName[vk::Space] = "Space";
    vkCodeToName[vk::Prior] = "PageUp";
    vkCodeToName[vk::Next] = "PageDown";
    vkCodeToName[vk::End] = "End";
    vkCodeToName[vk::Home] = "Home";
    vkCodeToName[vk::Left] = "Left";
    vkCodeToName[vk::Up] = "Up";
    vkCodeToName[vk::Right] = "Right";
    vkCodeToName[vk::Down] = "Down";
    vkCodeToName[vk::Snapshot] = "PrintScreen";
    vkCodeToName[vk::Insert] = "Insert";
    vkCodeToName[vk::Delete] = "Delete";
    vkCodeToName[vk::LWin] = "LeftWin";
    vkCodeToName[vk::RWin] = "RightWin";
    vkCodeToName[vk::Apps] = "Menu";
    vkCodeToName[vk::Numpad0] = "Numpad0";
    vkCodeToName[vk::Numpad1] = "Numpad1";
    vkCodeToName[vk::Numpad2] = "Numpad2";
    vkCodeToName[vk::Numpad3] = "Numpad3";
    vkCodeToName[vk::Numpad4] = "Numpad4";
    vkCodeToName[vk::Numpad5] = "Numpad5";
    vkCodeToName[vk::Numpad6] = "Numpad6";
    vkCodeToName[vk::Numpad7] = "Numpad7";
    vkCodeToName[vk::Numpad8] = "Numpad8";
    vkCodeToName[vk::Numpad9] = "Numpad9";
    vkCodeToName[vk::Multiply] = "Numpad*";
    vkCodeToName[vk::Add] = "Numpad+";
    vkCodeToName[vk::Subtract] = "Numpad-";
    vkCodeToName[vk::Decimal] = "Numpad.";
    vkCodeToName[vk::Divide] = "Numpad/";
    vkCodeToName[vk::F1] = "F1";
    vkCodeToName[vk::F2] = "F2";
    vkCodeToName[vk::F3] = "F3";
    vkCodeToName[vk::F4] = "F4";
    vkCodeToName[vk::F5] = "F5";
    vkCodeToName[vk::F6] = "F6";
    vkCodeToName[vk::F7] = "F7";
    vkCodeToName[vk::F8] = "F8";
    vkCodeToName[vk::F9] = "F9";
    vkCodeToName[vk::F10] = "F10";
    vkCodeToName[vk::F11] = "F11";
    vkCodeToName[vk::F12] = "F12";
    vkCodeToName[vk::NumLock] = "NumLock";
    vkCodeToName[vk::Scroll] = "ScrollLock";
    vkCodeToName[vk::LShift] = "LeftShift";
    vkCodeToName[vk::RShift] = "RightShift";
    vkCodeToName[vk::LControl] = "LeftCtrl";
    vkCodeToName[vk::RControl] = "RightCtrl";
    vkCodeToName[vk::LMenu] = "LeftAlt";
    vkCodeToName[vk::RMenu] = "RightAlt";
    vkCodeToName[vk::Oem1] = ";";
    vkCodeToName[vk::OemPlus] = "=";
    vkCodeToName[vk::OemComma] = ",";
    vkCodeToName[vk::OemMinus] = "-";
    vkCodeToName[vk::OemPeriod] = ".";
    vkCodeToName[vk::Oem2] = "/";
    vkCodeToName[vk::Oem3] = "`";
    vkCodeToName[vk::Oem4] = "[";
    vkCodeToName[vk::Oem5] = "\\";
    vkCodeToName[vk::Oem6] = "]";
    vkCodeToName[vk::Oem7] = "'";
}

std::string getKeyName(keyrecord::KeyCode vkCode) {
    auto it = vkCodeToName.find(vkCode);
    if (it != vkCodeToName.end()) {
        return it->second;
    }

    if ((vkCode >= 0x30 && vkCode <= 0x39) || (vkCode >= 0x41 && vkCode <= 0x5A)) {
        return std::string(1, static_cast<char>(vkCode));
    }

    return "Unknown_" + std::to_string(vkCode);
}
