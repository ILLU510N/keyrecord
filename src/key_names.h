#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

void initKeyMap();
std::string getKeyName(DWORD vkCode);
