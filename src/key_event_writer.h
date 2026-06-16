#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <string>

#include "config_path.h"

bool startWriter(const std::string& dbPath = keyrecord::getDefaultDatabasePath());
void enqueueKeyEvent(
    DWORD vkCode,
    std::chrono::system_clock::time_point eventTime = std::chrono::system_clock::now());
void stopWriter();
