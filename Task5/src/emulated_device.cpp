#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <chrono>
#include <random>
#include <sstream>
#include <ctime>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <termios.h>
    #include <sys/time.h>
#endif

#if defined(_WIN32)
#define PORT_NAME "COM3"
#else
#define PORT_NAME "/dev/pts/3"
#endif


float fetchTemperature() {
    unsigned long long milliseconds;
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    milliseconds = st.wMilliseconds + (st.wSecond * 1000) + (st.wMinute * 60000);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    milliseconds = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif

    std::mt19937 gen(milliseconds);

    std::uniform_real_distribution<> distrib(-30.0, 30.0);

    float baseTemperature = distrib(gen);

    std::uniform_real_distribution<> variationDistr(-0.5, 0.5);
    float variation = variationDistr(gen);

    return baseTemperature + variation;
}

class SimulatedDevice {
public:
    SimulatedDevice(const std::string& devicePort) : portName(devicePort) {
        initializePort();
    }

    ~SimulatedDevice() {
        terminatePort();
    }

    void startEmulation() {
        std::cout << "Starting device simulation on port: " << portName << std::endl;
        while (true) {
            float tempValue = fetchTemperature();

            std::ostringstream tempStream;
            tempStream << tempValue;

            sendToPort("t: [" + tempStream.str() + "]\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

private:
    std::string portName;

#if defined(_WIN32)
    HANDLE portDescriptor = INVALID_HANDLE_VALUE;

    void initializePort() {
        portDescriptor = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (portDescriptor == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Unable to access port: " + portName);
        }
    }

    void terminatePort() {
        if (portDescriptor != INVALID_HANDLE_VALUE) {
            CloseHandle(portDescriptor);
        }
    }

    void sendToPort(const std::string& message) {
        DWORD writtenBytes;
        WriteFile(portDescriptor, message.c_str(), message.size(), &writtenBytes, NULL);
    }
#else
    int portDescriptor = -1;

    void initializePort() {
        portDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY);
        if (portDescriptor < 0) {
            throw std::runtime_error("Unable to access port: " + portName);
        }

        struct termios config;
        tcgetattr(portDescriptor, &config);
        cfsetispeed(&config, B115200);
        cfsetospeed(&config, B115200);
        tcsetattr(portDescriptor, TCSANOW, &config);
    }

    void terminatePort() {
        if (portDescriptor >= 0) {
            close(portDescriptor);
        }
    }

    void sendToPort(const std::string& message) {
        write(portDescriptor, message.c_str(), message.size());
    }
#endif
};

int main() {
    try {
        SimulatedDevice device(PORT_NAME);
        device.startEmulation();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}
