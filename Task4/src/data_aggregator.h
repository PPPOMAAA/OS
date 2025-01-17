#ifndef DATA_AGGREGATOR_H
#define DATA_AGGREGATOR_H

#include <string>
#include <chrono>
#include <mutex>

enum class TimeResolution {
    DAY,
    HOUR,
    CURRENT
};

class DataAggregator {
private:
    std::string filename;
    TimeResolution resolution;
    std::mutex& fileMutex;

    std::string getCurrentTimestamp(TimeResolution res);
    std::chrono::system_clock::time_point getDefaultTime() const;

public:
    DataAggregator(const std::string& filename, TimeResolution res, std::mutex& mutex);

    void addTemperature(float temperature, const std::string& timestamp = "");

    float getAverageTemperature(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime);

    std::chrono::system_clock::time_point getFirstDate();
    std::chrono::system_clock::time_point getLastDate();

    void removeOutdated();
};

#endif