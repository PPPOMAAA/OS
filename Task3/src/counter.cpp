#include "counter.h"

Counter::Counter() : count(0) {}

void Counter::increase() {
    count.fetch_add(1);
}

void Counter::set_value(int value) {
    count.store(value);
}

int Counter::get_value() const {
    return count.load();
}
