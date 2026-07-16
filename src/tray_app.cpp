#include "tray_app.h"

#include "app_config.h"
#include "key_event_writer.h"
#include "key_names.h"
#include "platform/capture_backend.h"
#include "platform/platform_util.h"
#include "platform/tray.h"

int runKeyrecordApp() {
    // 数据库位置遵循配置文件（[storage] db_path / db_dir），未配置则用默认路径。
    if (!keyrecord::startWriter(keyrecord::resolveDatabasePath(), [] {
            keyrecord::requestStopCapture();
        })) {
        keyrecord::showError("Failed to initialize database writer");
        return 1;
    }

    if (!keyrecord::initializeTray([] {
            keyrecord::requestStopCapture();
        })) {
        keyrecord::stopWriter();
        keyrecord::showError("Failed to initialize tray integration");
        return 1;
    }

    int result = keyrecord::runCaptureLoop([](keyrecord::KeyCode vkCode, auto eventTime) {
        if (!keyrecord::enqueueKeyEvent(vkCode, eventTime)) {
            keyrecord::requestStopCapture();
        }
    });
    keyrecord::shutdownTray();
    keyrecord::stopWriter();
    return result != 0 || keyrecord::getWriterState() == keyrecord::WriterState::Failed ? 1 : 0;
}
