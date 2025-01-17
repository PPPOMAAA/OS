#include "server.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <string>
#include <vector>
#include <stdexcept>


Server::Server(int port, sqlite3* db) : PORT(port), serverSocket(INVALID_SOCKET), db(db) {}

Server::~Server() {
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }

#if defined(WIN32)
    WSACleanup();
#endif
}

bool Server::initialize() {
#if defined(WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return false;
    }
#endif

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        return false;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        return false;
    }
    return true;
}

void Server::run() {
    if (serverSocket == INVALID_SOCKET || !db) {
        std::cerr << "Server not initialized. Call initialize() first." << std::endl;
        return;
    }

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }

        std::thread clientThread([this, clientSocket]() { handleClient(clientSocket, db, dbMutex); });
        clientThread.detach();
    }
}

void Server::sendResponse(SOCKET clientSocket, const std::string& status, const std::string& body) {
    std::string response = "HTTP/1.1 " + status + "\r\n"
                             "Content-Type: text/plain\r\n"
                             "Connection: close\r\n\r\n" +
                             body;
    send(clientSocket, response.c_str(), response.length(), 0);
}

void Server::sendOkResponse(SOCKET clientSocket, const std::string& body) {
    sendResponse(clientSocket, "200 OK", body);
}

void Server::sendBadRequest(SOCKET clientSocket, const std::string& message) {
    sendResponse(clientSocket, "400 Bad Request", message);
}

void Server::sendNotFoundResponse(SOCKET clientSocket) {
    sendResponse(clientSocket, "404 Not Found", "Endpoint not found.");
}

std::string Server::handleLastRecordRequest(sqlite3* db, const std::string& table) {
    std::string responseBody = "No data found.";
    std::string sql = "SELECT * FROM \"" + table + "\" ORDER BY timestamp DESC LIMIT 1";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        responseBody = "Latest record:\n";
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double temperature = sqlite3_column_double(stmt, 1);
            responseBody += "Date: " + date + ", Temperature: " + std::to_string(temperature) + "\n";
        }
        sqlite3_finalize(stmt);
    } else {
        responseBody = "Error executing request";
    }
    return responseBody;
}

std::string Server::handleRangeRequest(sqlite3* db, const std::string& table, const std::string& start, const std::string& end) {
    std::string responseBody = "No data found.";
    std::string sql = "SELECT * FROM \"" + table + "\" WHERE timestamp BETWEEN ? AND ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, start.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, end.c_str(), -1, SQLITE_TRANSIENT);

        responseBody = "Temperature data:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double temperature = sqlite3_column_double(stmt, 1);
            responseBody += "Date: " + date + ", Temperature: " + std::to_string(temperature) + "\n";
        }
        sqlite3_finalize(stmt);
    } else {
        responseBody = "Error executing request";
    }
    return responseBody;
}

void Server::handleClient(SOCKET clientSocket, sqlite3* db, std::mutex& dbMutex) {
    char buffer[1024];
    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        std::cerr << "Failed to receive data from client." << std::endl;
        closesocket(clientSocket);
        return;
    }
    buffer[received] = '\0';

    std::cout << "Received request:\n" << buffer << std::endl;

    std::istringstream requestStream(buffer);
    std::string method, path, protocol;
    requestStream >> method >> path >> protocol;

    if (method == "GET" && path.find("/data?") != std::string::npos) {
        size_t queryPos = path.find('?');
        std::string query = path.substr(queryPos + 1);
        std::string table, start, end, lastRecordFlag;

        std::istringstream queryStream(query);
        std::string param;
        while (std::getline(queryStream, param, '&')) {
            size_t equalsPos = param.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = param.substr(0, equalsPos);
                std::string value = param.substr(equalsPos + 1);
                if (key == "table") {
                    table = value;
                } else if (key == "start") {
                    start = value;
                } else if (key == "end") {
                    end = value;
                } else if (key == "last") {
                    lastRecordFlag = value;
                }
            }
        }

        std::string responseBody;
        {
            std::lock_guard<std::mutex> lock(dbMutex);

            if (table.empty()) {
                sendBadRequest(clientSocket, "Missing table name.");
                return;
            }

            if (!lastRecordFlag.empty() && lastRecordFlag == "true") {

                responseBody = handleLastRecordRequest(db, table);

            } else if (!start.empty() && !end.empty()) {
                responseBody = handleRangeRequest(db, table, start, end);

            } else {
                sendBadRequest(clientSocket, "Invalid or missing parameters.");
                return;
            }
        }

        sendOkResponse(clientSocket, responseBody);
    } else {
        sendNotFoundResponse(clientSocket);
    }

    closesocket(clientSocket);
}