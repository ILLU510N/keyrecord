#pragma once

#include "platform/key_code.h"

#include <chrono>
#include <string>

#include "config_path.h"

bool startWriter(const std::string& dbPath = keyrecord::getDefaultDatabasePath());
void enqueueKeyEvent(
    keyrecord::KeyCode vkCode,
    std::chrono::system_clock::time_point eventTime = std::chrono::system_clock::now());
void stopWriter();
