#pragma once
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <array> // Add this if not already included

enum class LogLevel {
    INFO,
    WARNING,
    ERR,
    SECURITY
};

class Logger {
public:
    static Logger& getInstance();

    void enableConsoleOutput(bool enable);
    void log(LogLevel level, const std::string& message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getTimestamp();
    std::string getDateString();
    std::string levelToString(LogLevel level);
    std::string levelToFileName(LogLevel level, const std::string& date);
    void checkDateAndRotateLogs();

    std::mutex logMutex;
    std::string currentDate;
    bool consoleOutput = false;

    std::unordered_map<LogLevel, std::ofstream> logFiles;
};

namespace {
    constexpr std::array<LogLevel, 4> allLogLevels = {
        LogLevel::INFO,
        LogLevel::WARNING,
        LogLevel::ERR,
        LogLevel::SECURITY
    };
}

Logger::Logger() {
    currentDate = getDateString();
}

Logger::~Logger() {
    for (auto& [_, file] : logFiles) {
        if (file.is_open()) file.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::enableConsoleOutput(bool enable) {
    consoleOutput = enable;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    checkDateAndRotateLogs();

    std::string timestamp = getTimestamp();
    std::string levelStr = levelToString(level);
    std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] " + message + "\n";

    if (consoleOutput) std::cout << fullMessage;

    auto& file = logFiles[level];
    if (file.is_open()) {
        file << fullMessage;
        file.flush();
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &time);
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::getDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &time);
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d");
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::INFO: return "INFO";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERR: return "ERROR";
    case LogLevel::SECURITY: return "SECURITY";
    default: return "UNKNOWN";
    }
}

std::string Logger::levelToFileName(LogLevel level, const std::string& date) {
    std::string baseDir = "D:\\EduSync\\Logs";
    std::filesystem::create_directories(baseDir); // Ensure directory exists

    std::string levelName;
    switch (level) {
    case LogLevel::INFO: levelName = "info"; break;
    case LogLevel::WARNING: levelName = "warning"; break;
    case LogLevel::ERR: levelName = "error"; break;
    case LogLevel::SECURITY: levelName = "security"; break;
    default: levelName = "unknown"; break;
    }

    return baseDir + "\\" + levelName + "_" + date + ".log";
}

void Logger::checkDateAndRotateLogs() {
    std::string today = getDateString();
    if (today != currentDate) {
        // Close all old files
        for (auto& [_, file] : logFiles) {
            if (file.is_open()) file.close();
        }
        logFiles.clear();
        currentDate = today;
    }

    // Ensure today's files are open
    for (LogLevel level : allLogLevels) {
        if (logFiles.find(level) == logFiles.end()) {
            std::string filename = levelToFileName(level, currentDate);
            logFiles[level].open(filename, std::ios::app);
        }
    }
}