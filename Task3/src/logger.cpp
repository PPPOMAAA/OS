#include "logger.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

Logger::Logger(const char* filename) {
    log_file.open(filename, std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }

#ifdef _WIN32
    log_mutex = CreateMutex(nullptr, FALSE, nullptr);
    if (!log_mutex) {
        throw std::runtime_error("Failed to create mutex");
    }
#else
    pthread_mutex_init(&log_mutex, nullptr);
#endif
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }

#ifdef _WIN32
    CloseHandle(log_mutex);
#else
    pthread_mutex_destroy(&log_mutex);
#endif
}

void Logger::write_start_info(pid_type pid) {
    std::string time_buffer;
    get_current_time(time_buffer);
    write_log("[START] (" + time_buffer + ") PID: " + std::to_string(pid));
}

void Logger::write_status(pid_type pid, int counter) {
    std::string time_buffer;
    get_current_time(time_buffer);
    write_log("[STATUS] (" + time_buffer + ") PID: " + std::to_string(pid) + " Counter: " + std::to_string(counter));
}

void Logger::write_message(const char* message) {
    std::string time_buffer;
    get_current_time(time_buffer);
    write_log("[MESSAGE] (" + time_buffer + ") " + std::string(message));
}

void Logger::get_current_time(std::string& buffer) const {
#ifdef _WIN32
    SYSTEMTIME time;
    GetLocalTime(&time);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << time.wYear << "-"
        << std::setw(2) << std::setfill('0') << time.wMonth << "-"
        << std::setw(2) << std::setfill('0') << time.wDay << " "
        << std::setw(2) << std::setfill('0') << time.wHour << ":"
        << std::setw(2) << std::setfill('0') << time.wMinute << ":"
        << std::setw(2) << std::setfill('0') << time.wSecond << "."
        << std::setw(3) << std::setfill('0') << time.wMilliseconds;
    buffer = oss.str();
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm* time_info = localtime(&tv.tv_sec);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (time_info->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (time_info->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << time_info->tm_mday << " "
        << std::setw(2) << std::setfill('0') << time_info->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << time_info->tm_min << ":"
        << std::setw(2) << std::setfill('0') << time_info->tm_sec << "."
        << std::setw(3) << std::setfill('0') << (tv.tv_usec / 1000);
    buffer = oss.str();
#endif
}

void Logger::write_log(const std::string& message) {
#ifdef _WIN32
    WaitForSingleObject(log_mutex, INFINITE);
#else
    pthread_mutex_lock(&log_mutex);
#endif

    log_file << message << "\n";
    log_file.flush();

#ifdef _WIN32
    ReleaseMutex(log_mutex);
#else
    pthread_mutex_unlock(&log_mutex);
#endif
}