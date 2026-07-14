#include "platform/tray.h"

namespace keyrecord {

bool initializeTray(TrayExitCallback) {
    return true;
}

void shutdownTray() {
}

} // namespace keyrecord
