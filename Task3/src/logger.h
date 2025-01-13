#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
typedef DWORD pid_type;
#else
#include <unistd.h>
#include <pthread.h>
typedef pid_t pid_type;
#endif

class Logger {
public:
    explicit Logger(const char* filename);
    ~Logger();

    void write_start_info(pid_type pid);
    void write_status(pid_type pid, int counter);
    void write_message(const char* message);

private:
    std::ofstream log_file;
#ifdef _WIN32
    HANDLE log_mutex;
#else
    pthread_mutex_t log_mutex;
#endif

    void get_current_time(std::string& buffer) const;
    void write_log(const std::string& message);
};

#endif