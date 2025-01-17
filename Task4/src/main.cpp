#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <thread>
#include <mutex>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "data_aggregator.h"

#define DATA_CURRENT "data_current.txt"
#define DATA_HOUR "data_hour.txt"
#define DATA_DAY "day_day.txt"

#if defined(_WIN32)
    #include <windows.h>
    #include <tchar.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#define PORT_NAME L"COM4"
#else
#define PORT_NAME "/dev/pts/4" 
#endif

#define FLOAT_REGEX R"(\[([-+]?\d{1,2}\.\d+)\])"


#ifdef _WIN32
LPCSTR wstringToLPCSTR(const std::wstring& wideStr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) return nullptr;

    char* narrowStr = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, narrowStr, len, nullptr, nullptr);

    return narrowStr;
}
#endif

std::mutex dayMutex;
std::mutex hourMutex;
std::mutex currentMutex;

DataAggregator aggregatorDay(DATA_DAY, TimeResolution::DAY, dayMutex);
DataAggregator aggregatorHour(DATA_HOUR, TimeResolution::HOUR, hourMutex);
DataAggregator aggregatorCurrent(DATA_CURRENT, TimeResolution::CURRENT, currentMutex);


void monitorCurrentTemperature() {
#if defined(_WIN32)
    HANDLE portHandle = CreateFile(wstringToLPCSTR(PORT_NAME), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (portHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Unable to open the port: " << PORT_NAME << std::endl;
        return;
    }
    DWORD bytesRead;
#else
    int portFd = open(PORT_NAME, O_RDONLY);
    if (portFd < 0) {
        std::cerr << "Unable to open the port: " << PORT_NAME << std::endl;
        return;
    }
#endif

    char buffer[25];
    while (true) {
#if defined(_WIN32)
        if (!ReadFile(portHandle, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            std::cerr << "Error reading from port." << std::endl;
            break;
        }
#else
        int bytesRead = read(portFd, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0) {
            std::cerr << "Error reading from port." << std::endl;
            break;
        }
#endif
        buffer[bytesRead] = '\0';
        std::cout << buffer;
        std::string bufferStr(buffer);

        std::smatch lastMatch;
        std::regex floatRegex(FLOAT_REGEX);
        std::sregex_iterator it(bufferStr.begin(), bufferStr.end(), floatRegex);
        while (it != std::sregex_iterator()) {
            lastMatch = *it++;
        }
        if (lastMatch.size() > 1) {
                try {
                    float lastTemperature = std::stof(lastMatch.str(1));
                    aggregatorCurrent.addTemperature(lastTemperature);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid temperature format: " << lastMatch.str(1) << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Temperature out of range: " << lastMatch.str(1) << std::endl;
                }
        }

    }

#if defined(_WIN32)
    CloseHandle(portHandle);
#else
    close(portFd);
#endif
}

void monitorTemperature(
    DataAggregator& aggregatorSource,
    DataAggregator& aggregatorDest,
    std::chrono::hours timeStep,
    const std::string& aggregatorName) {

    auto firstTime = aggregatorSource.getFirstDate();
    auto lastTime = aggregatorDest.getLastDate() + timeStep;

    auto startTime = (firstTime > lastTime) ? firstTime : lastTime;

    std::time_t startTime_t = std::chrono::system_clock::to_time_t(startTime);
    std::tm* startTimeTM = std::localtime(&startTime_t);
    if (startTimeTM) {
        startTimeTM->tm_min = 0;
        startTimeTM->tm_sec = 0;
        if (timeStep == std::chrono::hours(24)) {
           startTimeTM->tm_hour = 0;
        }
        startTime = std::chrono::system_clock::from_time_t(mktime(startTimeTM));
    } else {
        std::cerr << "localtime failed\n";
        startTime = std::chrono::system_clock::now();
    }

    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::system_clock::to_time_t(now);

    std::tm* currentTimeTM = std::localtime(&currentTime);
    if (currentTimeTM) {
        currentTimeTM->tm_min = 0;
        currentTimeTM->tm_sec = 0;
        if (timeStep == std::chrono::hours(24)) {
            currentTimeTM->tm_hour = 0;
        }
    } else {
        std::cerr << "localtime failed\n";
        return;
    }

    auto endLoopTime = std::chrono::system_clock::from_time_t(mktime(currentTimeTM));

    for (auto currentTimePoint = startTime; currentTimePoint < endLoopTime; currentTimePoint += timeStep) {
        auto nextTimePoint = currentTimePoint + timeStep;
        float avgTemp = aggregatorSource.getAverageTemperature(currentTimePoint, nextTimePoint);

        std::time_t tt = std::chrono::system_clock::to_time_t(currentTimePoint);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");

        aggregatorDest.addTemperature(avgTemp, ss.str());
        std::cout << "Added to " << aggregatorName << " aggregator: " << ss.str() << " [" << avgTemp << "]" << std::endl;
    }
}


void monitorHourTemperature() {
    while (true) {
        monitorTemperature(aggregatorCurrent, aggregatorHour, std::chrono::hours(1), "hour");
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}


void monitorDayTemperature() {
    while (true) {
        monitorTemperature(aggregatorHour, aggregatorDay, std::chrono::hours(24), "day");
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
}


void removeUnactualTemperature() {
    while (true) {
        aggregatorCurrent.removeOutdated();
        aggregatorHour.removeOutdated();
        aggregatorDay.removeOutdated();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}


int main() {
    std::thread currentTemperatureThread(monitorCurrentTemperature);
    std::thread hourTemperatureThread(monitorHourTemperature);
    std::thread dayTemperatureThread(monitorDayTemperature);
    std::thread cleanTemperatureThread(removeUnactualTemperature);

    currentTemperatureThread.join();
    hourTemperatureThread.join();
    dayTemperatureThread.join();
    cleanTemperatureThread.join();

    return 0;
}