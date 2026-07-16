#pragma once

#include "platform/key_code.h"

#include <chrono>
#include <functional>
#include <string>

#include "config_path.h"

namespace keyrecord {

enum class WriterState {
    Stopped,
    Running,
    Failed,
};

using WriterFailureCallback = std::function<void()>;

bool startWriter(
    const std::string& dbPath = getDefaultDatabasePath(),
    WriterFailureCallback failureCallback = {});
bool enqueueKeyEvent(
    KeyCode vkCode,
    std::chrono::system_clock::time_point eventTime = std::chrono::system_clock::now());
void stopWriter();
WriterState getWriterState();

} // namespace keyrecord
