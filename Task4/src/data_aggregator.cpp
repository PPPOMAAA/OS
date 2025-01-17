#include "data_aggregator.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <random>

std::string DataAggregator::getCurrentTimestamp(TimeResolution res) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *tm_time = std::localtime(&in_time_t);

    if (res == TimeResolution::HOUR) {
        tm_time->tm_min = 0;
        tm_time->tm_sec = 0;
    } else if (res == TimeResolution::DAY) {
        tm_time->tm_hour = 0;
        tm_time->tm_min = 0;
        tm_time->tm_sec = 0;
    }

    std::stringstream ss;
    ss << std::put_time(tm_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

DataAggregator::DataAggregator(const std::string& filename, TimeResolution res, std::mutex& mutex) :
    filename(filename), resolution(res), fileMutex(mutex) {}

void DataAggregator::addTemperature(float temperature, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(fileMutex); 
    std::ofstream outfile(filename, std::ios_base::app);
    if (outfile.is_open()) {
        std::string timestampToWrite = timestamp;
        if (timestamp.empty()) {
            timestampToWrite = getCurrentTimestamp(resolution);
        } else {
            std::tm t{};
            std::istringstream ss(timestamp);
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                std::cerr << "Invalid timestamp format: " << timestamp << ". Using current time." << std::endl;
                timestampToWrite = getCurrentTimestamp(resolution);
            }
        }
        outfile << timestampToWrite << " [" << temperature << "]" << std::endl;
        outfile.close();
    } else {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}

float DataAggregator::getAverageTemperature(const std::chrono::system_clock::time_point& startTime, const std::chrono::system_clock::time_point& endTime) {
    std::lock_guard<std::mutex> lock(fileMutex); 
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return 0.0f;
    }

    std::vector<float> temperatures;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.length() >= 19) {
            std::tm t{};
            std::istringstream ss(line.substr(0, 19));
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) continue;

            std::time_t epochTime = mktime(&t);
            if (epochTime == -1) continue;

            auto fileTime = std::chrono::system_clock::from_time_t(epochTime);
            if (fileTime >= startTime && fileTime <= endTime) {
                size_t startPos = line.find('[') + 1;
                size_t endPos = line.find(']');
                if (startPos != std::string::npos && endPos != std::string::npos) {
                    try{
                        temperatures.push_back(std::stof(line.substr(startPos, endPos - startPos)));
                    } catch (const std::invalid_argument& e){
                        std::cerr << "Invalid argument: " << e.what() << '\n';
                        continue;
                    } catch (const std::out_of_range& e){
                        std::cerr << "Out of range: " << e.what() << '\n';
                        continue;
                    }
                }
            }
        }
    }
    infile.close();

    if (temperatures.empty()) return 0.0f;
    float sum = std::accumulate(temperatures.begin(), temperatures.end(), 0.0f);
    return sum / temperatures.size();
}


std::chrono::system_clock::time_point DataAggregator::getFirstDate() {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return std::chrono::system_clock::now();
    }

    std::string line;
    if (std::getline(infile, line)) {
        if (line.length() >= 19) { 
            std::tm t{};
            std::istringstream ss(line.substr(0, 19));
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
            if (!ss.fail()) { 
                std::time_t epochTime = mktime(&t);
                if (epochTime != -1) { 
                    infile.close();
                    return std::chrono::system_clock::from_time_t(epochTime); 
                } else {
                    std::cerr << "mktime failed\n";
                }
            } else {
                std::cerr << "Parsing date failed\n";
            }
        } else {
            std::cerr << "Line too short to contain date\n";
        }
    }

    infile.close();
    return std::chrono::system_clock::now(); 
}

std::chrono::system_clock::time_point DataAggregator::getLastDate() {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return getDefaultTime();
    }

    std::string line;
    std::chrono::system_clock::time_point lastDate = std::chrono::system_clock::now(); 
    bool dateFound = false;

    while (std::getline(infile, line)) {
        if (line.length() >= 19) {
            std::tm t{};
            std::istringstream ss(line.substr(0, 19));
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

            if (!ss.fail()) {
                std::time_t epochTime = mktime(&t);
                if (epochTime != -1) {
                    lastDate = std::chrono::system_clock::from_time_t(epochTime);
                    dateFound = true;
                } else {
                    std::cerr << "mktime failed\n";
                }
            } else {
                std::cerr << "Parsing date failed for line: " << line << "\n";
            }
        }
    }

    infile.close();

    if (!dateFound) {
        return getDefaultTime();
    }

    return lastDate;
}

std::chrono::system_clock::time_point DataAggregator::getDefaultTime() const {
    auto now = std::chrono::system_clock::now();
    switch (resolution) {
    case TimeResolution::HOUR:
        return now - std::chrono::hours(24);
    case TimeResolution::DAY:
        return now - std::chrono::hours(24 * 30);
    default:
        return now;
    }
}

void DataAggregator::removeOutdated() {
    std::lock_guard<std::mutex> lock(fileMutex); 
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::string line;

    auto now = std::chrono::system_clock::now();
    std::chrono::seconds timeThreshold;
    switch (resolution) {
    case TimeResolution::DAY:
        timeThreshold = std::chrono::hours(24 * 365);
        break;
    case TimeResolution::HOUR:
        timeThreshold = std::chrono::hours(24 * 30);
        break;
    case TimeResolution::CURRENT:
        timeThreshold = std::chrono::hours(24);
        break;
    }

    if (std::getline(infile, line)) {
        if (line.length() >= 19) {
            std::tm t{};
            std::istringstream ss(line.substr(0, 19));
            ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                std::cerr << "Parsing failed\n";
            }
            std::time_t epochTime = mktime(&t);
            if (epochTime == -1) {
                std::cerr << "mktime failed\n";
            }

            auto fileTime = std::chrono::system_clock::from_time_t(epochTime);

            if (now - fileTime <= timeThreshold) {
                infile.close();
                return;
            }
        }

        lines.push_back(line);
    }

    while (std::getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    bool stillUnActual = true;
    for (const auto& l : lines) {
        if (l.length() >= 19) {
            if (stillUnActual) {
                std::tm t{};
                std::istringstream ss(l.substr(0, 19));
                ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
                if (ss.fail()) {
                    std::cerr << "Parsing failed\n";
                    continue;
                }
                std::time_t epochTime = mktime(&t);
                if (epochTime == -1) {
                    std::cerr << "mktime failed\n";
                    continue;
                }

                auto fileTime = std::chrono::system_clock::from_time_t(epochTime);
                
                if (now - fileTime <= timeThreshold) {
                    outfile << l << std::endl;
                    stillUnActual = false;
                }
            } else {
                outfile << l << std::endl;
            }
        }
    }
    outfile.close();
}