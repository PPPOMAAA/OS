#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "logger.h"
#include "counter.h"

class ProcessManager {
public:
    explicit ProcessManager(Logger& log_instance);
    void handle_subprocesses(Counter& counter);

private:
    Logger& logger;
    bool copy_running;
};

#endif