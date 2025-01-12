#ifndef PROCESS_LIB_H
#define PROCESS_LIB_H

#include <string>
#include <optional>

namespace process {

bool start_process(const std::string &command);

std::optional<int> wait_for_process();

}

#endif
