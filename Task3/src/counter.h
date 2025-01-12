#ifndef COUNTER_H
#define COUNTER_H

#include <atomic>

class Counter {
public:
    Counter();
    void increase();
    void set_value(int value);
    int get_value() const;

private:
    std::atomic<int> count;
};

#endif