#include "process_manager.h"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/mman.h>
#include <iostream>

ProcessManager::ProcessManager(Logger& log_instance) : logger(log_instance), copy_running(false) {}

void ProcessManager::handle_subprocesses(Counter& counter) {
    if (copy_running) {
        logger.write_message("Subprocess still running. Skipping new launch.");
        return;
    }

    copy_running = true;

    int* shared_counter = static_cast<int*>(mmap(
        nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    *shared_counter = counter.get_value();

    pid_t pid1 = fork();
    if (pid1 == 0) {
        pid_t child_pid = getpid();
        logger.write_message(("Subprocess 1 started. PID: " + std::to_string(child_pid)).c_str());
        *shared_counter += 10;
        logger.write_message(("Subprocess 1 exited. PID: " + std::to_string(child_pid)).c_str());
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        pid_t child_pid = getpid();
        logger.write_message(("Subprocess 2 started. PID: " + std::to_string(child_pid)).c_str());
        *shared_counter *= 2;
        sleep(2);
        *shared_counter /= 2;
        logger.write_message(("Subprocess 2 exited. PID: " + std::to_string(child_pid)).c_str());
        exit(0);
    }

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    counter.set_value(*shared_counter);

    munmap(shared_counter, sizeof(int));

    copy_running = false;
}
