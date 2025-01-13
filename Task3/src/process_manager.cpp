#include "process_manager.h"
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <string>
#else
#include <sys/mman.h>
#endif

ProcessManager::ProcessManager(Logger& log_instance) : logger(log_instance), copy_running(false) {}

void ProcessManager::handle_subprocesses(Counter& counter) {
    if (copy_running) {
        logger.write_message("Subprocess still running. Skipping new launch.");
        return;
    }

    copy_running = true;

#ifdef _WIN32
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi1, pi2;

    std::string cmd1 = "prog.exe --child 1";
    if (CreateProcess(nullptr, cmd1.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi1)) {
        logger.write_message(("Subprocess 1 started. PID: " + std::to_string(pi1.dwProcessId)).c_str());
        WaitForSingleObject(pi1.hProcess, INFINITE);
        logger.write_message(("Subprocess 1 exited. PID: " + std::to_string(pi1.dwProcessId)).c_str());
        CloseHandle(pi1.hProcess);
        CloseHandle(pi1.hThread);
    }

    std::string cmd2 = "prog.exe --child 2";
    if (CreateProcess(nullptr, cmd2.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi2)) {
        logger.write_message(("Subprocess 2 started. PID: " + std::to_string(pi2.dwProcessId)).c_str());
        WaitForSingleObject(pi2.hProcess, INFINITE);
        logger.write_message(("Subprocess 2 exited. PID: " + std::to_string(pi2.dwProcessId)).c_str());
        CloseHandle(pi2.hProcess);
        CloseHandle(pi2.hThread);
    }
#else
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execlp("./prog", "prog", "--child", "1", nullptr);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execlp("./prog", "prog", "--child", "2", nullptr);
        exit(0);
    }

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
#endif

    copy_running = false;
}
