#include "key_event_writer.h"

#include "key_names.h"
#include "platform/platform_util.h"

#include <sqlite3.h>

#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr size_t BATCH_SIZE = 100;
constexpr auto FLUSH_INTERVAL = std::chrono::seconds(1);
constexpr int MAX_WRITE_ATTEMPTS = 3;
constexpr auto WRITE_RETRY_DELAY = std::chrono::milliseconds(100);

struct KeyEvent {
    keyrecord::KeyCode vkCode;
    std::chrono::system_clock::time_point eventTime;
};

sqlite3* db = nullptr;
sqlite3_stmt* insertStmt = nullptr;
sqlite3_stmt* upsertDailyKeyStmt = nullptr;
sqlite3_stmt* upsertDailyHourStmt = nullptr;
std::mutex queueMutex;
std::condition_variable queueCv;
std::deque<KeyEvent> keyQueue;
std::thread writerThread;
bool writerStopRequested = false;
keyrecord::WriterState writerState = keyrecord::WriterState::Stopped;
keyrecord::WriterFailureCallback writerFailureCallback;

void logError(const std::string& message) {
    keyrecord::debugLog("KeyRecord: " + message + "\n");
}

bool execSQL(const char* sql) {
    char* error = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::string message = error ? error : sqlite3_errmsg(db);
        logError("SQL execution failed: " + message);
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool backfillSummaryTables();

bool initDB(const std::string& dbPath) {
    if (db) {
        return true;
    }

    // sqlite3_open 不会创建缺失的父目录；若配置的 db_path/db_dir 指向尚不存在的
    // 目录，需先建好目录，否则打开数据库会直接失败导致采集端无法启动。
    std::string dirError;
    if (!keyrecord::ensureParentDirectoryExists(dbPath, &dirError)) {
        logError("prepare database directory failed: " + dirError);
        return false;
    }

    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        logError("open database failed: " + std::string(db ? sqlite3_errmsg(db) : "unknown"));
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
        return false;
    }

    if (!execSQL("PRAGMA journal_mode=WAL;") ||
        !execSQL("PRAGMA busy_timeout=3000;") ||
        !execSQL("CREATE TABLE IF NOT EXISTS keys("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                 "timestamp INTEGER,"
                 "date TEXT,"
                 "hour INTEGER,"
                 "vk_code INTEGER,"
                 "key_name TEXT);") ||
        !execSQL("CREATE TABLE IF NOT EXISTS daily_key_stats("
                 "date TEXT NOT NULL,"
                 "vk_code INTEGER NOT NULL,"
                 "key_name TEXT NOT NULL,"
                 "count INTEGER NOT NULL DEFAULT 0,"
                 "PRIMARY KEY(date, vk_code, key_name));") ||
        !execSQL("CREATE TABLE IF NOT EXISTS daily_hour_stats("
                 "date TEXT NOT NULL,"
                 "hour INTEGER NOT NULL,"
                 "count INTEGER NOT NULL DEFAULT 0,"
                 "PRIMARY KEY(date, hour));") ||
        !execSQL("CREATE TABLE IF NOT EXISTS keyrecord_meta("
                 "name TEXT PRIMARY KEY,"
                 "value TEXT NOT NULL);") ||
        !execSQL("CREATE INDEX IF NOT EXISTS idx_keys_date ON keys(date);") ||
        !execSQL("CREATE INDEX IF NOT EXISTS idx_keys_date_vk_name ON keys(date, vk_code, key_name);") ||
        !execSQL("CREATE INDEX IF NOT EXISTS idx_keys_key_name_vk_code ON keys(key_name, vk_code);")) {
        return false;
    }

    if (!backfillSummaryTables()) {
        return false;
    }

    const char* insertSql =
        "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES(?,?,?,?,?)";
    rc = sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr);
    if (rc != SQLITE_OK) {
        logError("prepare insert SQL failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    const char* upsertDailyKeySql =
        "INSERT INTO daily_key_stats(date,vk_code,key_name,count) VALUES(?,?,?,1) "
        "ON CONFLICT(date,vk_code,key_name) DO UPDATE SET count = count + 1";
    rc = sqlite3_prepare_v2(db, upsertDailyKeySql, -1, &upsertDailyKeyStmt, nullptr);
    if (rc != SQLITE_OK) {
        logError("prepare daily key summary SQL failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    const char* upsertDailyHourSql =
        "INSERT INTO daily_hour_stats(date,hour,count) VALUES(?,?,1) "
        "ON CONFLICT(date,hour) DO UPDATE SET count = count + 1";
    rc = sqlite3_prepare_v2(db, upsertDailyHourSql, -1, &upsertDailyHourStmt, nullptr);
    if (rc != SQLITE_OK) {
        logError("prepare daily hour summary SQL failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    return true;
}

void closeDB() {
    if (upsertDailyHourStmt) {
        int rc = sqlite3_finalize(upsertDailyHourStmt);
        if (rc != SQLITE_OK) {
            logError("finalize daily hour summary statement failed: " + std::string(sqlite3_errmsg(db)));
        }
        upsertDailyHourStmt = nullptr;
    }

    if (upsertDailyKeyStmt) {
        int rc = sqlite3_finalize(upsertDailyKeyStmt);
        if (rc != SQLITE_OK) {
            logError("finalize daily key summary statement failed: " + std::string(sqlite3_errmsg(db)));
        }
        upsertDailyKeyStmt = nullptr;
    }

    if (insertStmt) {
        int rc = sqlite3_finalize(insertStmt);
        if (rc != SQLITE_OK) {
            logError("finalize insert statement failed: " + std::string(sqlite3_errmsg(db)));
        }
        insertStmt = nullptr;
    }

    if (db) {
        int rc = sqlite3_close(db);
        if (rc != SQLITE_OK) {
            logError("Failed to close database: " + std::string(sqlite3_errmsg(db)));
        } else {
            db = nullptr;
        }
    }
}

bool bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    int rc = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        logError("Failed to bind text parameter: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

bool backfillSummaryTables() {
    sqlite3_stmt* markerStatement = nullptr;
    const char* markerSql =
        "SELECT 1 FROM keyrecord_meta WHERE name = 'summary_backfill_v1' LIMIT 1";
    if (sqlite3_prepare_v2(db, markerSql, -1, &markerStatement, nullptr) != SQLITE_OK) {
        logError("prepare summary backfill marker query failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    const int markerResult = sqlite3_step(markerStatement);
    sqlite3_finalize(markerStatement);
    if (markerResult == SQLITE_ROW) {
        return true;
    }
    if (markerResult != SQLITE_DONE) {
        logError("query summary backfill marker failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    // 首次升级时只补齐缺失汇总行，完成后写入版本标记，避免每次启动全表聚合。
    return execSQL("INSERT OR IGNORE INTO daily_key_stats(date,vk_code,key_name,count) "
                   "SELECT date,vk_code,key_name,COUNT(*) FROM keys "
                   "WHERE date IS NOT NULL AND vk_code IS NOT NULL AND key_name IS NOT NULL "
                   "GROUP BY date,vk_code,key_name;") &&
           execSQL("INSERT OR IGNORE INTO daily_hour_stats(date,hour,count) "
                   "SELECT date,hour,COUNT(*) FROM keys "
                   "WHERE date IS NOT NULL AND hour IS NOT NULL "
                   "GROUP BY date,hour;") &&
           execSQL("INSERT INTO keyrecord_meta(name,value) VALUES('summary_backfill_v1','complete') "
                   "ON CONFLICT(name) DO UPDATE SET value = excluded.value;");
}

bool resetStatement(sqlite3_stmt* stmt, const char* label) {
    int rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        logError("Failed to reset " + std::string(label) + " statement: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    rc = sqlite3_clear_bindings(stmt);
    if (rc != SQLITE_OK) {
        logError("Failed to clear " + std::string(label) + " bindings: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    return true;
}

bool updateSummaries(const char* date, int hour, keyrecord::KeyCode vkCode, const std::string& keyName) {
    if (!resetStatement(upsertDailyKeyStmt, "daily key summary")) {
        return false;
    }
    if (!bindText(upsertDailyKeyStmt, 1, date) ||
        sqlite3_bind_int(upsertDailyKeyStmt, 2, static_cast<int>(vkCode)) != SQLITE_OK ||
        !bindText(upsertDailyKeyStmt, 3, keyName)) {
        logError("Failed to bind daily key summary parameters: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    int rc = sqlite3_step(upsertDailyKeyStmt);
    if (rc != SQLITE_DONE) {
        logError("Failed to update daily key summary: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    if (!resetStatement(upsertDailyHourStmt, "daily hour summary")) {
        return false;
    }
    if (!bindText(upsertDailyHourStmt, 1, date) ||
        sqlite3_bind_int(upsertDailyHourStmt, 2, hour) != SQLITE_OK) {
        logError("Failed to bind daily hour summary parameters: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    rc = sqlite3_step(upsertDailyHourStmt);
    if (rc != SQLITE_DONE) {
        logError("Failed to update daily hour summary: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    return true;
}

bool insertEvent(const KeyEvent& event) {
    auto epoch = event.eventTime.time_since_epoch();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
    auto t = std::chrono::system_clock::to_time_t(event.eventTime);
    std::tm local = {};
    if (!keyrecord::localTime(local, t)) {
        logError("Failed to convert event time to local time");
        return false;
    }

    char date[32];
    std::snprintf(date, sizeof(date), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    std::string keyName = keyrecord::getKeyName(event.vkCode);

    int rc = sqlite3_reset(insertStmt);
    if (rc != SQLITE_OK) {
        logError("Failed to reset insert statement: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    rc = sqlite3_clear_bindings(insertStmt);
    if (rc != SQLITE_OK) {
        logError("Failed to clear insert bindings: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    if (sqlite3_bind_int64(insertStmt, 1, static_cast<sqlite3_int64>(ts)) != SQLITE_OK ||
        sqlite3_bind_int(insertStmt, 3, local.tm_hour) != SQLITE_OK ||
        sqlite3_bind_int(insertStmt, 4, static_cast<int>(event.vkCode)) != SQLITE_OK) {
        logError("Failed to bind numeric parameter: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    if (!bindText(insertStmt, 2, date) ||
        !bindText(insertStmt, 5, keyName)) {
        return false;
    }

    rc = sqlite3_step(insertStmt);
    if (rc != SQLITE_DONE) {
        logError("Failed to insert key record: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    return updateSummaries(date, local.tm_hour, event.vkCode, keyName);
}

bool writeBatch(const std::vector<KeyEvent>& batch) {
    if (batch.empty()) {
        return true;
    }

    // 批量写入使用短事务，避免每个按键都触发一次事务提交。
    if (!execSQL("BEGIN IMMEDIATE TRANSACTION;")) {
        return false;
    }

    for (const auto& event : batch) {
        if (!insertEvent(event)) {
            execSQL("ROLLBACK;");
            return false;
        }
    }

    if (!execSQL("COMMIT;")) {
        execSQL("ROLLBACK;");
        return false;
    }

    return true;
}

bool writeBatchWithRetry(const std::vector<KeyEvent>& batch) {
    for (int attempt = 1; attempt <= MAX_WRITE_ATTEMPTS; ++attempt) {
        if (writeBatch(batch)) {
            return true;
        }

        const int errorCode = db ? sqlite3_extended_errcode(db) : SQLITE_ERROR;
        if ((errorCode & 0xFF) != SQLITE_BUSY && (errorCode & 0xFF) != SQLITE_LOCKED) {
            return false;
        }
        if (attempt < MAX_WRITE_ATTEMPTS) {
            std::this_thread::sleep_for(WRITE_RETRY_DELAY * attempt);
        }
    }
    return false;
}

void writerLoop() {
    while (true) {
        std::vector<KeyEvent> batch;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [] {
                return writerStopRequested || !keyQueue.empty();
            });

            if (keyQueue.empty() && writerStopRequested) {
                break;
            }

            // 最多等待 1 秒聚合批次；达到 100 条则立即写入，控制异常退出时的数据窗口。
            auto deadline = std::chrono::steady_clock::now() + FLUSH_INTERVAL;
            while (!writerStopRequested && keyQueue.size() < BATCH_SIZE) {
                if (queueCv.wait_until(lock, deadline) == std::cv_status::timeout) {
                    break;
                }
            }

            size_t count = writerStopRequested ? keyQueue.size() : std::min(keyQueue.size(), BATCH_SIZE);
            batch.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                batch.push_back(keyQueue.front());
                keyQueue.pop_front();
            }
        }

        if (!writeBatchWithRetry(batch)) {
            keyrecord::WriterFailureCallback failureCallback;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                // 保留未写入批次，允许同一进程修复数据库问题后重新启动 writer 重试。
                for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                    keyQueue.push_front(*it);
                }
                writerState = keyrecord::WriterState::Failed;
                writerStopRequested = true;
                failureCallback = writerFailureCallback;
            }
            logError("Writer entered failed state; capture must stop");
            if (failureCallback) {
                failureCallback();
            }
            break;
        }
    }
}

} // namespace

namespace keyrecord {

bool startWriter(const std::string& dbPath, WriterFailureCallback failureCallback) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (writerThread.joinable()) {
        return writerState == WriterState::Running;
    }

    writerStopRequested = false;
    writerFailureCallback = std::move(failureCallback);
    if (!initDB(dbPath)) {
        closeDB();
        writerFailureCallback = {};
        if (writerState != WriterState::Failed) {
            writerState = WriterState::Stopped;
        }
        return false;
    }

    // queueMutex 仍由当前线程持有，新线程在进入循环前无法观察到中间状态。
    writerState = WriterState::Running;
    try {
        writerThread = std::thread(writerLoop);
    } catch (const std::exception& ex) {
        logError("Failed to start writer thread: " + std::string(ex.what()));
        writerState = WriterState::Stopped;
        writerFailureCallback = {};
        closeDB();
        return false;
    }

    return true;
}

bool enqueueKeyEvent(KeyCode vkCode, std::chrono::system_clock::time_point eventTime) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (writerState != WriterState::Running || writerStopRequested) {
            logError("Writer is not running; rejecting key event");
            return false;
        }
        keyQueue.push_back({vkCode, eventTime});
    }
    queueCv.notify_one();
    return true;
}

void stopWriter() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        writerStopRequested = true;
    }
    queueCv.notify_one();

    if (writerThread.joinable()) {
        writerThread.join();
    }

    closeDB();

    std::lock_guard<std::mutex> lock(queueMutex);
    if (writerState == WriterState::Running) {
        writerState = WriterState::Stopped;
    }
    writerFailureCallback = {};
}

WriterState getWriterState() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return writerState;
}

} // namespace keyrecord
