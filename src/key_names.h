#pragma once

#include "platform/key_code.h"

#include <string>

void initKeyMap();
std::string getKeyName(keyrecord::KeyCode vkCode);
