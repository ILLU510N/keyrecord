#include "key_names.h"

#include "platform/virtual_keys.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace keyrecord {
namespace {

using KeyNameEntry = std::pair<KeyCode, std::string_view>;

constexpr auto KEY_NAMES = std::to_array<KeyNameEntry>({
    {vk::Back, "Backspace"}, {vk::Tab, "Tab"}, {vk::Return, "Enter"},
    {vk::Shift, "Shift"}, {vk::Control, "Ctrl"}, {vk::Menu, "Alt"},
    {vk::Pause, "Pause"}, {vk::Capital, "CapsLock"}, {vk::Escape, "Esc"},
    {vk::Space, "Space"}, {vk::Prior, "PageUp"}, {vk::Next, "PageDown"},
    {vk::End, "End"}, {vk::Home, "Home"}, {vk::Left, "Left"}, {vk::Up, "Up"},
    {vk::Right, "Right"}, {vk::Down, "Down"}, {vk::Snapshot, "PrintScreen"},
    {vk::Insert, "Insert"}, {vk::Delete, "Delete"}, {vk::LWin, "LeftWin"},
    {vk::RWin, "RightWin"}, {vk::Apps, "Menu"},
    {vk::Numpad0, "Numpad0"}, {vk::Numpad1, "Numpad1"}, {vk::Numpad2, "Numpad2"},
    {vk::Numpad3, "Numpad3"}, {vk::Numpad4, "Numpad4"}, {vk::Numpad5, "Numpad5"},
    {vk::Numpad6, "Numpad6"}, {vk::Numpad7, "Numpad7"}, {vk::Numpad8, "Numpad8"},
    {vk::Numpad9, "Numpad9"}, {vk::Multiply, "Numpad*"}, {vk::Add, "Numpad+"},
    {vk::Subtract, "Numpad-"}, {vk::Decimal, "Numpad."}, {vk::Divide, "Numpad/"},
    {vk::F1, "F1"}, {vk::F2, "F2"}, {vk::F3, "F3"}, {vk::F4, "F4"},
    {vk::F5, "F5"}, {vk::F6, "F6"}, {vk::F7, "F7"}, {vk::F8, "F8"},
    {vk::F9, "F9"}, {vk::F10, "F10"}, {vk::F11, "F11"}, {vk::F12, "F12"},
    {vk::NumLock, "NumLock"}, {vk::Scroll, "ScrollLock"},
    {vk::LShift, "LeftShift"}, {vk::RShift, "RightShift"},
    {vk::LControl, "LeftCtrl"}, {vk::RControl, "RightCtrl"},
    {vk::LMenu, "LeftAlt"}, {vk::RMenu, "RightAlt"},
    {vk::Oem1, ";"}, {vk::OemPlus, "="}, {vk::OemComma, ","}, {vk::OemMinus, "-"},
    {vk::OemPeriod, "."}, {vk::Oem2, "/"}, {vk::Oem3, "`"}, {vk::Oem4, "["},
    {vk::Oem5, "\\"}, {vk::Oem6, "]"}, {vk::Oem7, "'"},
});

} // namespace

std::string getKeyName(KeyCode vkCode) {
    const auto it = std::lower_bound(KEY_NAMES.begin(), KEY_NAMES.end(), vkCode,
                                     [](const KeyNameEntry& entry, KeyCode code) {
                                         return entry.first < code;
                                     });
    if (it != KEY_NAMES.end() && it->first == vkCode) {
        return std::string(it->second);
    }

    if ((vkCode >= 0x30 && vkCode <= 0x39) || (vkCode >= 0x41 && vkCode <= 0x5A)) {
        return std::string(1, static_cast<char>(vkCode));
    }

    return "Unknown_" + std::to_string(vkCode);
}

} // namespace keyrecord
