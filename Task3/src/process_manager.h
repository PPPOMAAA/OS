#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "logger.h"
#include "counter.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

class ProcessManager {
public:
    explicit ProcessManager(Logger& log_instance);
    void handle_subprocesses(Counter& counter);

private:
    Logger& logger;
    bool copy_running;

#ifdef _WIN32
    void create_subprocess(const char* program, const char* args, Counter& counter, int operation);
#endif
};

#endif