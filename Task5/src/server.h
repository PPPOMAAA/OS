#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <mutex>

#if defined(WIN32)
#   include <winsock2.h>
#   include <ws2tcpip.h>
    typedef int socklen_t;
    #pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <unistd.h>
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define SOCKET_ERROR -1
#endif

#include <sqlite3.h>

class Server {
private:
    SOCKET serverSocket;
    sqlite3* db;
    std::mutex dbMutex;
    const int PORT;
    const std::string DB_PATH;

    void sendResponse(SOCKET clientSocket, const std::string& status, const std::string& body);
    void sendOkResponse(SOCKET clientSocket, const std::string& body);
    void sendBadRequest(SOCKET clientSocket, const std::string& message);
    void sendNotFoundResponse(SOCKET clientSocket);

    std::string handleLastRecordRequest(sqlite3* db, const std::string& table);
    std::string handleRangeRequest(sqlite3* db, const std::string& table, const std::string& start, const std::string& end);
    void handleClient(SOCKET clientSocket, sqlite3* db, std::mutex& dbMutex);

public:
    Server(int port, sqlite3* db);
    ~Server();

    bool initialize();
    void run();
};

#endif