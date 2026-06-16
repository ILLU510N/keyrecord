#include "key_names.h"

#include <map>

namespace {

std::map<DWORD, std::string> vkCodeToName;

} // namespace

void initKeyMap() {
    vkCodeToName[VK_BACK] = "Backspace";
    vkCodeToName[VK_TAB] = "Tab";
    vkCodeToName[VK_RETURN] = "Enter";
    vkCodeToName[VK_SHIFT] = "Shift";
    vkCodeToName[VK_CONTROL] = "Ctrl";
    vkCodeToName[VK_MENU] = "Alt";
    vkCodeToName[VK_PAUSE] = "Pause";
    vkCodeToName[VK_CAPITAL] = "CapsLock";
    vkCodeToName[VK_ESCAPE] = "Esc";
    vkCodeToName[VK_SPACE] = "Space";
    vkCodeToName[VK_PRIOR] = "PageUp";
    vkCodeToName[VK_NEXT] = "PageDown";
    vkCodeToName[VK_END] = "End";
    vkCodeToName[VK_HOME] = "Home";
    vkCodeToName[VK_LEFT] = "Left";
    vkCodeToName[VK_UP] = "Up";
    vkCodeToName[VK_RIGHT] = "Right";
    vkCodeToName[VK_DOWN] = "Down";
    vkCodeToName[VK_SNAPSHOT] = "PrintScreen";
    vkCodeToName[VK_INSERT] = "Insert";
    vkCodeToName[VK_DELETE] = "Delete";
    vkCodeToName[VK_LWIN] = "LeftWin";
    vkCodeToName[VK_RWIN] = "RightWin";
    vkCodeToName[VK_APPS] = "Menu";
    vkCodeToName[VK_NUMPAD0] = "Numpad0";
    vkCodeToName[VK_NUMPAD1] = "Numpad1";
    vkCodeToName[VK_NUMPAD2] = "Numpad2";
    vkCodeToName[VK_NUMPAD3] = "Numpad3";
    vkCodeToName[VK_NUMPAD4] = "Numpad4";
    vkCodeToName[VK_NUMPAD5] = "Numpad5";
    vkCodeToName[VK_NUMPAD6] = "Numpad6";
    vkCodeToName[VK_NUMPAD7] = "Numpad7";
    vkCodeToName[VK_NUMPAD8] = "Numpad8";
    vkCodeToName[VK_NUMPAD9] = "Numpad9";
    vkCodeToName[VK_MULTIPLY] = "Numpad*";
    vkCodeToName[VK_ADD] = "Numpad+";
    vkCodeToName[VK_SUBTRACT] = "Numpad-";
    vkCodeToName[VK_DECIMAL] = "Numpad.";
    vkCodeToName[VK_DIVIDE] = "Numpad/";
    vkCodeToName[VK_F1] = "F1";
    vkCodeToName[VK_F2] = "F2";
    vkCodeToName[VK_F3] = "F3";
    vkCodeToName[VK_F4] = "F4";
    vkCodeToName[VK_F5] = "F5";
    vkCodeToName[VK_F6] = "F6";
    vkCodeToName[VK_F7] = "F7";
    vkCodeToName[VK_F8] = "F8";
    vkCodeToName[VK_F9] = "F9";
    vkCodeToName[VK_F10] = "F10";
    vkCodeToName[VK_F11] = "F11";
    vkCodeToName[VK_F12] = "F12";
    vkCodeToName[VK_NUMLOCK] = "NumLock";
    vkCodeToName[VK_SCROLL] = "ScrollLock";
    vkCodeToName[VK_LSHIFT] = "LeftShift";
    vkCodeToName[VK_RSHIFT] = "RightShift";
    vkCodeToName[VK_LCONTROL] = "LeftCtrl";
    vkCodeToName[VK_RCONTROL] = "RightCtrl";
    vkCodeToName[VK_LMENU] = "LeftAlt";
    vkCodeToName[VK_RMENU] = "RightAlt";
    vkCodeToName[VK_OEM_1] = ";";
    vkCodeToName[VK_OEM_PLUS] = "=";
    vkCodeToName[VK_OEM_COMMA] = ",";
    vkCodeToName[VK_OEM_MINUS] = "-";
    vkCodeToName[VK_OEM_PERIOD] = ".";
    vkCodeToName[VK_OEM_2] = "/";
    vkCodeToName[VK_OEM_3] = "`";
    vkCodeToName[VK_OEM_4] = "[";
    vkCodeToName[VK_OEM_5] = "\\";
    vkCodeToName[VK_OEM_6] = "]";
    vkCodeToName[VK_OEM_7] = "'";
}

std::string getKeyName(DWORD vkCode) {
    auto it = vkCodeToName.find(vkCode);
    if (it != vkCodeToName.end()) {
        return it->second;
    }

    if ((vkCode >= 0x30 && vkCode <= 0x39) || (vkCode >= 0x41 && vkCode <= 0x5A)) {
        return std::string(1, static_cast<char>(vkCode));
    }

    return "Unknown_" + std::to_string(vkCode);
}
