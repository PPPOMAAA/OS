#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <pthread.h>
#include <unistd.h>

class Logger {
public:
    explicit Logger(const char* filename);
    void write_start_info(pid_t pid);
    void write_status(pid_t pid, int counter);
    void write_message(const char* message);

private:
    std::ofstream file;
    pthread_mutex_t file_lock;
    void get_current_time(char* buffer, size_t size);
    void write_log(const std::string& message);
};

#endif