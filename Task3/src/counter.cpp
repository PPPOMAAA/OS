#include "counter.h"

Counter::Counter(const char* shm_name) : shared_name(shm_name) {
#ifdef _WIN32
    shm_handle = CreateFileMapping(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(int), shared_name);
    if (!shm_handle) {
        throw std::runtime_error("Failed to create or open shared memory");
    }

    shared_counter = static_cast<int*>(MapViewOfFile(
        shm_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int)));
    if (!shared_counter) {
        throw std::runtime_error("Failed to map shared memory");
    }

    if (*shared_counter == 0) {
        *shared_counter = 0;
    }
#else
    shm_fd = shm_open(shared_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("Failed to open shared memory");
    }

    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        throw std::runtime_error("Failed to set size for shared memory");
    }

    shared_counter = static_cast<int*>(mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (shared_counter == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory");
    }

    if (*shared_counter == 0) {
        *shared_counter = 0;
    }
#endif
}

Counter::~Counter() {
#ifdef _WIN32
    UnmapViewOfFile(shared_counter);
    CloseHandle(shm_handle);
#else
    munmap(shared_counter, sizeof(int));
    close(shm_fd);
    shm_unlink(shared_name);
#endif
}

void Counter::increase() {
    (*shared_counter)++;
}

void Counter::set_value(int value) {
    *shared_counter = value;
}

int Counter::get_value() const {
    return *shared_counter;
}
