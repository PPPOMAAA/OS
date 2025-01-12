#include "process_lib.h"
#include <cstdlib>
#include <stdexcept>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace process {

static int last_pid = -1;

bool start_process(const std::string &command) {
#ifdef PLATFORM_WINDOWS
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi;
    if (!CreateProcess(nullptr, const_cast<char *>(command.c_str()), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    last_pid = static_cast<int>(pi.dwProcessId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        std::exit(EXIT_FAILURE);
    } else if (pid > 0) {
        last_pid = pid;
        return true;
    } else {
        return false;
    }
#endif
}

std::optional<int> wait_for_process() {
    if (last_pid == -1) {
        return std::nullopt;
    }
#ifdef PLATFORM_WINDOWS
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(last_pid));
    if (!process) {
        return std::nullopt;
    }
    WaitForSingleObject(process, INFINITE);
    DWORD exit_code;
    if (!GetExitCodeProcess(process, &exit_code)) {
        CloseHandle(process);
        return std::nullopt;
    }
    CloseHandle(process);
    return static_cast<int>(exit_code);
#else
    int status;
    if (waitpid(last_pid, &status, 0) == -1) {
        return std::nullopt;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return std::nullopt;
    }
#endif
}
}