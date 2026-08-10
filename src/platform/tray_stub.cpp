#include "platform/tray.h"

namespace keyrecord {

bool initializeTray(TrayExitCallback, TrayOpenVisualizationCallback) {
    return true;
}

void shutdownTray() {
}

} // namespace keyrecord
