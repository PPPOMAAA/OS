#include "process_lib.h"
#include <iostream>
#include <string>

int main() {
    std::string command;
    std::cout << "Enter a command to run in the background: ";
    std::getline(std::cin, command);

    if (process::start_process(command)) {
        std::cout << "Process started successfully. Waiting for it to complete...\n";
        auto exit_code = process::wait_for_process();
        if (exit_code.has_value()) {
            std::cout << "Process finished with exit code: " << *exit_code << "\n";
        } else {
            std::cout << "Failed to retrieve the exit code.\n";
        }
    } else {
        std::cout << "Failed to start the process.\n";
    }

    return 0;
}