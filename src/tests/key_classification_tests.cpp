#include "../key_classification.h"

#include <iostream>
#include <string_view>

namespace {

struct RegionCase {
    int vkCode;
    keyrecord::KeyRegion expected;
    const char* message;
};

struct HandCase {
    int vkCode;
    keyrecord::KeyHand expected;
    const char* message;
};

bool expectRegion(int vkCode, keyrecord::KeyRegion expected, const char* message) {
    const keyrecord::KeyRegion actual = keyrecord::getKeyRegion(vkCode);
    if (actual != expected) {
        std::cerr << message << "\nvk_code: " << vkCode << "\nexpected: " << keyrecord::regionName(expected)
                  << "\nactual: " << keyrecord::regionName(actual) << "\n";
        return false;
    }
    return true;
}

bool expectHand(int vkCode, keyrecord::KeyHand expected, const char* message) {
    const keyrecord::KeyHand actual = keyrecord::getKeyHand(vkCode);
    if (actual != expected) {
        std::cerr << message << "\nvk_code: " << vkCode << "\nexpected: " << keyrecord::handName(expected)
                  << "\nactual: " << keyrecord::handName(actual) << "\n";
        return false;
    }
    return true;
}

bool expectName(std::string_view actual, std::string_view expected, const char* message) {
    if (actual != expected) {
        std::cerr << message << "\nexpected: " << expected << "\nactual: " << actual << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    bool ok = true;

    const RegionCase regionCases[] = {
        {'A', keyrecord::KeyRegion::Letters, "A should be classified as letters"},
        {'Z', keyrecord::KeyRegion::Letters, "Z should be classified as letters"},
        {'0', keyrecord::KeyRegion::Digits, "0 should be classified as digits"},
        {'9', keyrecord::KeyRegion::Digits, "9 should be classified as digits"},
        {96, keyrecord::KeyRegion::Numpad, "Numpad0 should be classified as numpad"},
        {111, keyrecord::KeyRegion::Numpad, "Numpad divide should be classified as numpad"},
        {112, keyrecord::KeyRegion::Function, "F1 should be classified as function"},
        {123, keyrecord::KeyRegion::Function, "F12 should be classified as function"},
        {33, keyrecord::KeyRegion::Navigation, "PageUp should be classified as navigation"},
        {40, keyrecord::KeyRegion::Navigation, "Down should be classified as navigation"},
        {45, keyrecord::KeyRegion::Navigation, "Insert should be classified as navigation"},
        {46, keyrecord::KeyRegion::Navigation, "Delete should be classified as navigation"},
        {16, keyrecord::KeyRegion::Modifiers, "Shift should be classified as modifiers"},
        {17, keyrecord::KeyRegion::Modifiers, "Ctrl should be classified as modifiers"},
        {18, keyrecord::KeyRegion::Modifiers, "Alt should be classified as modifiers"},
        {20, keyrecord::KeyRegion::Modifiers, "CapsLock should be classified as modifiers"},
        {91, keyrecord::KeyRegion::Modifiers, "LeftWin should be classified as modifiers"},
        {92, keyrecord::KeyRegion::Modifiers, "RightWin should be classified as modifiers"},
        {144, keyrecord::KeyRegion::Modifiers, "NumLock should be classified as modifiers"},
        {145, keyrecord::KeyRegion::Modifiers, "ScrollLock should be classified as modifiers"},
        {160, keyrecord::KeyRegion::Modifiers, "LeftShift should be classified as modifiers"},
        {165, keyrecord::KeyRegion::Modifiers, "RightAlt should be classified as modifiers"},
        {8, keyrecord::KeyRegion::Control, "Backspace should be classified as control"},
        {9, keyrecord::KeyRegion::Control, "Tab should be classified as control"},
        {13, keyrecord::KeyRegion::Control, "Enter should be classified as control"},
        {27, keyrecord::KeyRegion::Control, "Esc should be classified as control"},
        {32, keyrecord::KeyRegion::Control, "Space should be classified as control"},
        {186, keyrecord::KeyRegion::Punctuation, "Semicolon should be classified as punctuation"},
        {192, keyrecord::KeyRegion::Punctuation, "Backtick should be classified as punctuation"},
        {219, keyrecord::KeyRegion::Punctuation, "Left bracket should be classified as punctuation"},
        {222, keyrecord::KeyRegion::Punctuation, "Quote should be classified as punctuation"},
        {-1, keyrecord::KeyRegion::Other, "Negative vk_code should be classified as other"},
        {95, keyrecord::KeyRegion::Other, "Value before numpad range should be classified as other"},
        {124, keyrecord::KeyRegion::Other, "Value after function range should be classified as other"},
        {193, keyrecord::KeyRegion::Other, "Gap after OEM punctuation range should be classified as other"},
        {223, keyrecord::KeyRegion::Other, "Value after bracket punctuation range should be classified as other"},
        {255, keyrecord::KeyRegion::Other, "Unknown vk_code should be classified as other"},
    };

    for (const RegionCase& test : regionCases) {
        ok = expectRegion(test.vkCode, test.expected, test.message) && ok;
    }

    const HandCase handCases[] = {
        {'Q', keyrecord::KeyHand::Left, "Q should be classified as left hand"},
        {'T', keyrecord::KeyHand::Left, "T should be classified as left hand"},
        {'A', keyrecord::KeyHand::Left, "A should be classified as left hand"},
        {'G', keyrecord::KeyHand::Left, "G should be classified as left hand"},
        {'Z', keyrecord::KeyHand::Left, "Z should be classified as left hand"},
        {'B', keyrecord::KeyHand::Left, "B should be classified as left hand"},
        {'1', keyrecord::KeyHand::Left, "1 should be classified as left hand"},
        {'5', keyrecord::KeyHand::Left, "5 should be classified as left hand"},
        {192, keyrecord::KeyHand::Left, "Backtick should be classified as left hand"},
        {27, keyrecord::KeyHand::Left, "Esc should be classified as left hand"},
        {9, keyrecord::KeyHand::Left, "Tab should be classified as left hand"},
        {20, keyrecord::KeyHand::Left, "CapsLock should be classified as left hand"},
        {160, keyrecord::KeyHand::Left, "LeftShift should be classified as left hand"},
        {162, keyrecord::KeyHand::Left, "LeftCtrl should be classified as left hand"},
        {164, keyrecord::KeyHand::Left, "LeftAlt should be classified as left hand"},
        {112, keyrecord::KeyHand::Left, "F1 should be classified as left hand"},
        {117, keyrecord::KeyHand::Left, "F6 should be classified as left hand"},
        {'Y', keyrecord::KeyHand::Right, "Y should be classified as right hand"},
        {'P', keyrecord::KeyHand::Right, "P should be classified as right hand"},
        {'H', keyrecord::KeyHand::Right, "H should be classified as right hand"},
        {'L', keyrecord::KeyHand::Right, "L should be classified as right hand"},
        {'N', keyrecord::KeyHand::Right, "N should be classified as right hand"},
        {'M', keyrecord::KeyHand::Right, "M should be classified as right hand"},
        {'6', keyrecord::KeyHand::Right, "6 should be classified as right hand"},
        {'0', keyrecord::KeyHand::Right, "0 should be classified as right hand"},
        {189, keyrecord::KeyHand::Right, "Minus should be classified as right hand"},
        {222, keyrecord::KeyHand::Right, "Quote should be classified as right hand"},
        {8, keyrecord::KeyHand::Right, "Backspace should be classified as right hand"},
        {13, keyrecord::KeyHand::Right, "Enter should be classified as right hand"},
        {161, keyrecord::KeyHand::Right, "RightShift should be classified as right hand"},
        {163, keyrecord::KeyHand::Right, "RightCtrl should be classified as right hand"},
        {165, keyrecord::KeyHand::Right, "RightAlt should be classified as right hand"},
        {118, keyrecord::KeyHand::Right, "F7 should be classified as right hand"},
        {123, keyrecord::KeyHand::Right, "F12 should be classified as right hand"},
        {96, keyrecord::KeyHand::Right, "Numpad0 should be classified as right hand"},
        {111, keyrecord::KeyHand::Right, "Numpad divide should be classified as right hand"},
        {144, keyrecord::KeyHand::Right, "NumLock should be classified as right hand"},
        {33, keyrecord::KeyHand::Right, "PageUp should be classified as right hand"},
        {40, keyrecord::KeyHand::Right, "Down should be classified as right hand"},
        {45, keyrecord::KeyHand::Right, "Insert should be classified as right hand"},
        {46, keyrecord::KeyHand::Right, "Delete should be classified as right hand"},
        {32, keyrecord::KeyHand::Both, "Space should be classified as both hands"},
        {16, keyrecord::KeyHand::Unknown, "Generic Shift should be classified as unknown hand"},
        {17, keyrecord::KeyHand::Unknown, "Generic Ctrl should be classified as unknown hand"},
        {18, keyrecord::KeyHand::Unknown, "Generic Alt should be classified as unknown hand"},
        {91, keyrecord::KeyHand::Unknown, "LeftWin should be classified as unknown hand"},
        {92, keyrecord::KeyHand::Unknown, "RightWin should be classified as unknown hand"},
        {-1, keyrecord::KeyHand::Unknown, "Negative vk_code should be classified as unknown hand"},
        {255, keyrecord::KeyHand::Unknown, "Unknown vk_code should be classified as unknown hand"},
    };

    for (const HandCase& test : handCases) {
        ok = expectHand(test.vkCode, test.expected, test.message) && ok;
    }

    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Letters), "letters", "Letters name mismatch") && ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Digits), "digits", "Digits name mismatch") && ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Numpad), "numpad", "Numpad name mismatch") && ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Function), "function", "Function name mismatch") &&
         ok;
    ok = expectName(
             keyrecord::regionName(keyrecord::KeyRegion::Navigation),
             "navigation",
             "Navigation name mismatch") &&
         ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Modifiers), "modifiers", "Modifiers name mismatch") &&
         ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Control), "control", "Control name mismatch") && ok;
    ok = expectName(
             keyrecord::regionName(keyrecord::KeyRegion::Punctuation),
             "punctuation",
             "Punctuation name mismatch") &&
         ok;
    ok = expectName(keyrecord::regionName(keyrecord::KeyRegion::Other), "other", "Other name mismatch") && ok;
    ok = expectName(
             keyrecord::regionName(static_cast<keyrecord::KeyRegion>(999)),
             "other",
             "Invalid region should fall back to other") &&
         ok;

    ok = expectName(keyrecord::handName(keyrecord::KeyHand::Left), "left", "Left hand name mismatch") && ok;
    ok = expectName(keyrecord::handName(keyrecord::KeyHand::Right), "right", "Right hand name mismatch") && ok;
    ok = expectName(keyrecord::handName(keyrecord::KeyHand::Both), "both", "Both hand name mismatch") && ok;
    ok = expectName(keyrecord::handName(keyrecord::KeyHand::Unknown), "unknown", "Unknown hand name mismatch") && ok;
    ok = expectName(
             keyrecord::handName(static_cast<keyrecord::KeyHand>(999)),
             "unknown",
             "Invalid hand should fall back to unknown") &&
         ok;

    return ok ? 0 : 1;
}
