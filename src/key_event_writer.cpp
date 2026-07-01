#include "key_event_writer.h"

#include "key_names.h"

#include <sqlite3.h>

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr size_t BATCH_SIZE = 100;
constexpr auto FLUSH_INTERVAL = std::chrono::seconds(1);

struct KeyEvent {
    DWORD vkCode;
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

void logError(const std::string& message) {
    OutputDebugStringA(("KeyRecord: " + message + "\n").c_str());
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
    // 只补齐缺失汇总行，避免未来明细归档后覆盖长期保留的汇总结果。
    return execSQL("INSERT OR IGNORE INTO daily_key_stats(date,vk_code,key_name,count) "
                   "SELECT date,vk_code,key_name,COUNT(*) FROM keys "
                   "WHERE date IS NOT NULL AND vk_code IS NOT NULL AND key_name IS NOT NULL "
                   "GROUP BY date,vk_code,key_name;") &&
           execSQL("INSERT OR IGNORE INTO daily_hour_stats(date,hour,count) "
                   "SELECT date,hour,COUNT(*) FROM keys "
                   "WHERE date IS NOT NULL AND hour IS NOT NULL "
                   "GROUP BY date,hour;");
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

bool updateSummaries(const char* date, int hour, DWORD vkCode, const std::string& keyName) {
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
    tm local;
    localtime_s(&local, &t);

    char date[32];
    sprintf_s(date, "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    std::string keyName = getKeyName(event.vkCode);

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

        writeBatch(batch);
    }
}

} // namespace

bool startWriter(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (writerThread.joinable()) {
        return true;
    }

    writerStopRequested = false;
    if (!initDB(dbPath)) {
        closeDB();
        return false;
    }

    try {
        writerThread = std::thread(writerLoop);
    } catch (const std::exception& ex) {
        logError("Failed to start writer thread: " + std::string(ex.what()));
        closeDB();
        return false;
    }

    return true;
}

void enqueueKeyEvent(DWORD vkCode, std::chrono::system_clock::time_point eventTime) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (writerStopRequested) {
            logError("Writer thread is stopping; dropping key event");
            return;
        }
        keyQueue.push_back({vkCode, eventTime});
    }
    queueCv.notify_one();
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
}
