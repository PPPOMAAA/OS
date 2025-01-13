#include "logger.h"
#include "process_manager.h"
#include "counter.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

volatile bool running = true;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}

void sleep_ms(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--child") {
        if (argc != 3) {
            std::cerr << "Invalid arguments for child process\n";
            return 1;
        }

        int child_type = std::stoi(argv[2]);
        Counter counter;
        Logger logger("log.txt");

        pid_type pid =
#ifdef _WIN32
            GetCurrentProcessId();
#else
            getpid();
#endif

        if (child_type == 1) {
            logger.write_message(("Subprocess 1 started. PID: " + std::to_string(pid)).c_str());
            counter.set_value(counter.get_value() + 10);
            logger.write_message(("Subprocess 1 exited. PID: " + std::to_string(pid)).c_str());
        } else if (child_type == 2) {
            logger.write_message(("Subprocess 2 started. PID: " + std::to_string(pid)).c_str());
            counter.set_value(counter.get_value() * 2);
            sleep_ms(2000);
            counter.set_value(counter.get_value() / 2);
            logger.write_message(("Subprocess 2 exited. PID: " + std::to_string(pid)).c_str());
        }

        return 0;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    Logger logger("log.txt");
    ProcessManager processManager(logger);
    Counter counter;

    pid_type pid =
#ifdef _WIN32
        GetCurrentProcessId();
#else
        getpid();
#endif

    logger.write_start_info(pid);

#ifdef _WIN32
    HANDLE counter_thread = CreateThread(nullptr, 0, [](LPVOID arg) -> DWORD {
        Counter* counter = static_cast<Counter*>(arg);
        while (running) {
            sleep_ms(300);
            counter->increase();
        }
        return 0;
    }, &counter, 0, nullptr);

    HANDLE logger_thread = CreateThread(nullptr, 0, [](LPVOID arg) -> DWORD {
        auto* args = static_cast<std::pair<Logger*, Counter*>*>(arg);
        Logger* logger = args->first;
        Counter* counter = args->second;
        pid_type pid = GetCurrentProcessId();
        while (running) {
            sleep_ms(1000);
            logger->write_status(pid, counter->get_value());
        }
        return 0;
    }, new std::pair<Logger*, Counter*>(&logger, &counter), 0, nullptr);

    HANDLE process_thread = CreateThread(nullptr, 0, [](LPVOID arg) -> DWORD {
        auto* args = static_cast<std::pair<ProcessManager*, Counter*>*>(arg);
        ProcessManager* processManager = args->first;
        Counter* counter = args->second;
        while (running) {
            sleep_ms(3000);
            processManager->handle_subprocesses(*counter);
        }
        return 0;
    }, new std::pair<ProcessManager*, Counter*>(&processManager, &counter), 0, nullptr);

    std::cout << "Enter a new counter value or type 'exit' to quit:\n";
    std::string input;
    while (running && std::cin >> input) {
        if (input == "exit") {
            running = false;
            break;
        }
        try {
            int value = std::stoi(input);
            counter.set_value(value);
            logger.write_message(("Counter set by user: " + std::to_string(value)).c_str());
        } catch (...) {
            std::cout << "Invalid input. Try again.\n";
        }
    }

    WaitForSingleObject(counter_thread, INFINITE);
    WaitForSingleObject(logger_thread, INFINITE);
    WaitForSingleObject(process_thread, INFINITE);
#else
    pthread_t counter_thread, logger_thread, process_thread;

    pthread_create(&counter_thread, nullptr, [](void* arg) -> void* {
        Counter* counter = static_cast<Counter*>(arg);
        while (running) {
            sleep_ms(300);
            counter->increase();
        }
        return nullptr;
    }, &counter);

    pthread_create(&logger_thread, nullptr, [](void* arg) -> void* {
        auto* args = static_cast<std::pair<Logger*, Counter*>*>(arg);
        Logger* logger = args->first;
        Counter* counter = args->second;
        pid_type pid = getpid();
        while (running) {
            sleep_ms(1000);
            logger->write_status(pid, counter->get_value());
        }
        return nullptr;
    }, new std::pair<Logger*, Counter*>(&logger, &counter));

    pthread_create(&process_thread, nullptr, [](void* arg) -> void* {
        auto* args = static_cast<std::pair<ProcessManager*, Counter*>*>(arg);
        ProcessManager* processManager = args->first;
        Counter* counter = args->second;
        while (running) {
            sleep_ms(3000);
            processManager->handle_subprocesses(*counter);
        }
        return nullptr;
    }, new std::pair<ProcessManager*, Counter*>(&processManager, &counter));

    std::cout << "Enter a new counter value or type 'exit' to quit:\n";
    std::string input;
    while (running && std::cin >> input) {
        if (input == "exit") {
            running = false;
            break;
        }
        try {
            int value = std::stoi(input);
            counter.set_value(value);
            logger.write_message(("Counter set by user: " + std::to_string(value)).c_str());
        } catch (...) {
            std::cout << "Invalid input. Try again.\n";
        }
    }

    pthread_join(counter_thread, nullptr);
    pthread_join(logger_thread, nullptr);
    pthread_join(process_thread, nullptr);
#endif

    return 0;
}
