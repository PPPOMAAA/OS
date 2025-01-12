#include "logger.h"
#include "process_manager.h"
#include "counter.h"
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

volatile bool running = true;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    Logger logger("log.txt");
    ProcessManager processManager(logger);
    Counter counter;

    pid_t pid = getpid();
    logger.write_start_info(pid);

    pthread_t counter_thread, logger_thread, process_thread;

    auto update_counter = [](void* arg) -> void* {
        Counter* counter = static_cast<Counter*>(arg);
        while (running) {
            usleep(300000); // 300 мс
            counter->increase();
        }
        return nullptr;
    };

    auto log_status = [](void* arg) -> void* {
        auto* args = static_cast<std::pair<Logger*, Counter*>*>(arg);
        Logger* logger = args->first;
        Counter* counter = args->second;
        pid_t pid = getpid();
        while (running) {
            usleep(1000000); // 1 с
            logger->write_status(pid, counter->get_value());
        }
        return nullptr;
    };

    auto manage_processes = [](void* arg) -> void* {
        auto* args = static_cast<std::pair<ProcessManager*, Counter*>*>(arg);
        ProcessManager* processManager = args->first;
        Counter* counter = args->second;
        while (running) {
            usleep(3000000); // 3 с
            processManager->handle_subprocesses(*counter);
            counter->set_value(counter->get_value());
    }
    return nullptr;
};


    std::pair<Logger*, Counter*> logger_args = {&logger, &counter};
    std::pair<ProcessManager*, Counter*> process_args = {&processManager, &counter};

    pthread_create(&counter_thread, nullptr, update_counter, &counter);
    pthread_create(&logger_thread, nullptr, log_status, &logger_args);
    pthread_create(&process_thread, nullptr, manage_processes, &process_args);

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

    return 0;
}
