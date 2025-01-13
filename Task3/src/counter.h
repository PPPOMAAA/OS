#ifndef COUNTER_H
#define COUNTER_H

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdexcept>

class Counter {
public:
    Counter(const char* shm_name = "/shared_counter");
    ~Counter();

    void increase();
    void set_value(int value);
    int get_value() const;

private:
    const char* shared_name;

#ifdef _WIN32
    HANDLE shm_handle;
    int* shared_counter;
#else
    int shm_fd;
    int* shared_counter;
#endif
};

#endif