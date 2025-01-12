#include "logger.h"
#include <sys/time.h>
#include <ctime>
#include <cstring>
#include <iostream>

Logger::Logger(const char* filename) {
    file.open(filename, std::ios::app);
    pthread_mutex_init(&file_lock, nullptr);
}

void Logger::write_start_info(pid_t pid) {
    char time_buffer[64];
    get_current_time(time_buffer, sizeof(time_buffer));
    write_log("[START] (" + std::string(time_buffer) + ") PID: " + std::to_string(pid));
}

void Logger::write_status(pid_t pid, int counter) {
    char time_buffer[64];
    get_current_time(time_buffer, sizeof(time_buffer));
    write_log("[STATUS] (" + std::string(time_buffer) + ") PID: " + std::to_string(pid) + " Counter: " + std::to_string(counter));
}

void Logger::write_message(const char* message) {
    char time_buffer[64];
    get_current_time(time_buffer, sizeof(time_buffer));
    write_log("[MESSAGE] (" + std::string(time_buffer) + ") "+ message);
}

void Logger::get_current_time(char* buffer, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    struct tm* time_info = localtime(&tv.tv_sec);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
             time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
             tv.tv_usec / 1000);
}

void Logger::write_log(const std::string& message) {
    pthread_mutex_lock(&file_lock);
    file << message << "\n";
    file.flush();
    pthread_mutex_unlock(&file_lock);
}