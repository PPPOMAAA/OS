#include "data_aggregator.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <random>

sqlite3*& getDatabase() {
    static sqlite3* db = nullptr;
    static std::once_flag once;

    std::call_once(once, []() {
        const char* dbPath = "logs.db";
        int rc = sqlite3_open(dbPath, &db);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            getDatabase();
        }
    });

    return db;
}

std::string DataAggregator::getCurrentTimestamp(TimeResolution res) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *tm_time = std::localtime(&in_time_t);

    if (res == TimeResolution::HOUR) {
        tm_time->tm_min = 0;
        tm_time->tm_sec = 0;
    } else if (res == TimeResolution::DAY) {
        tm_time->tm_hour = 0;
        tm_time->tm_min = 0;
        tm_time->tm_sec = 0;
    }

    std::stringstream ss;
    ss << std::put_time(tm_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}


DataAggregator::DataAggregator(const std::string& filename, TimeResolution res, std::mutex& mutex) :
    filename(filename), resolution(res), fileMutex(mutex), db(getDatabase()), timeThreshold(std::chrono::hours(0)) {

    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS \"" << filename << "\" (timestamp TEXT PRIMARY KEY, temperature REAL)";
    std::string create_table_sql = ss.str();
    const char* sql_cstr = create_table_sql.c_str();

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql_cstr, nullptr, nullptr, &errmsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
    switch (resolution) {
        case TimeResolution::DAY:
            timeThreshold = std::chrono::hours(24 * 365);
            break;
        case TimeResolution::HOUR:
            timeThreshold = std::chrono::hours(24 * 30);
            break;
        case TimeResolution::CURRENT:
            timeThreshold = std::chrono::hours(24);
            break;
    }
}


void DataAggregator::addTemperature(float temperature, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!db) return;

    std::stringstream ss;
    std::string timestampToInsert;

    if (timestamp.empty()) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_time = *std::localtime(&time_t_now);

        std::stringstream ss_time;
        ss_time << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
        timestampToInsert = ss_time.str();
    } else {
        timestampToInsert = timestamp;
    }

    ss << "INSERT OR REPLACE INTO \"" << filename << "\" (timestamp, temperature) VALUES (?, ?)";

    std::string sql = ss.str();
    const char* sql_cstr = sql.c_str();

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql_cstr, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, timestampToInsert.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, temperature);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_OK && rc != SQLITE_ROW) {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}


DataAggregator::~DataAggregator() {}


float DataAggregator::getAverageTemperature(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime) {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!db) return 0.0f;

    std::stringstream ss;
    ss << "SELECT AVG(temperature) FROM \"" << filename << "\" WHERE timestamp BETWEEN ? AND ?"; // placeholders ?

    std::string sql = ss.str();
    const char* sql_cstr = sql.c_str();

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql_cstr, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return 0.0f;
    }

    auto formatTime = [](const std::chrono::system_clock::time_point& time) {
        auto time_t_time = std::chrono::system_clock::to_time_t(time);
        std::tm tm_time = *std::localtime(&time_t_time);
        std::stringstream ss_time;
        ss_time << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
        return ss_time.str();
    };

    std::string startTimeStr = formatTime(startTime);
    std::string endTimeStr = formatTime(endTime);

    sqlite3_bind_text(stmt, 1, startTimeStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, endTimeStr.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        float avgTemp = static_cast<float>(sqlite3_column_double(stmt, 0));
        sqlite3_finalize(stmt);
        return avgTemp;
    } else if (rc != SQLITE_DONE) {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return 0.0f;
}


std::chrono::system_clock::time_point DataAggregator::getFirstDate() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!db) return std::chrono::system_clock::now();

    std::stringstream ss;
    ss << "SELECT MIN(timestamp) FROM \"" << filename << "\"";

    std::string sql = ss.str();
    const char* sql_cstr = sql.c_str();

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql_cstr, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return std::chrono::system_clock::now();
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (timestamp) {
        std::tm t{};
        std::istringstream ss_ts(timestamp);
        ss_ts >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

        if (!ss_ts.fail()) {
            std::time_t epochTime = mktime(&t);
            if (epochTime != -1) {
            return std::chrono::system_clock::from_time_t(epochTime);
            } else {
            std::cerr << "mktime failed\n";
            }
        } else {
            std::cerr << "Parsing date failed from SQLite result\n";
        }
        }
    } else if (rc == SQLITE_DONE) {
        std::cerr << "No records found in database\n";
        return std::chrono::system_clock::now();
    } else {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return std::chrono::system_clock::now();
}


std::chrono::system_clock::time_point DataAggregator::getLastDate() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!db) {
        return getDefaultTime();
    }

    std::stringstream ss;
    ss << "SELECT MAX(timestamp) FROM \"" << filename << "\"";
    std::string sql = ss.str();
    const char* sql_cstr = sql.c_str();

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql_cstr, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return getDefaultTime();
    }

    rc = sqlite3_step(stmt);
    std::chrono::system_clock::time_point result = std::chrono::system_clock::time_point::min();

    if (rc == SQLITE_ROW) {
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (timestamp) {
            std::tm t{};
            std::istringstream ss_ts(timestamp);
            ss_ts >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

            if (!ss_ts.fail()) {
                std::time_t epochTime = mktime(&t);
                if (epochTime != -1) {
                    result = std::chrono::system_clock::from_time_t(epochTime);
                    sqlite3_finalize(stmt);
                    return result;
                } else {
                    std::cerr << "mktime failed\n";
                }
            } else {
                std::cerr << "Parsing date failed from SQLite result\n";
            }
        }
    } else if (rc != SQLITE_DONE && rc != SQLITE_OK && rc != SQLITE_ROW) {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return getDefaultTime();
}

std::chrono::system_clock::time_point DataAggregator::getDefaultTime() const {
    auto now = std::chrono::system_clock::now();
    if (resolution == TimeResolution::HOUR) {
        return now - std::chrono::hours(24);
    } else if (resolution == TimeResolution::DAY) {
        return now - std::chrono::hours(24*30);
    }
    return now;
}

void DataAggregator::removeOutdated() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!db) return;

    std::stringstream ss;
    ss << "DELETE FROM \"" << filename << "\" WHERE timestamp < datetime('now', '-";

    switch (resolution) {
        case TimeResolution::DAY:
            ss << "1 year')";
            break;
        case TimeResolution::HOUR:
            ss << "1 month')";
            break;
        case TimeResolution::CURRENT:
            ss << "1 day')";
            break;
    }

    std::string sql = ss.str();
    const char* sql_cstr = sql.c_str();

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql_cstr, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        std::cerr << "SQL execute error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}