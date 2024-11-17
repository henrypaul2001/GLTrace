#pragma once
#include <glm/ext/vector_float4.hpp>
#include <vector>
#include <iostream>
struct LogEntry {
public:
	LogEntry(const char* type, const char* log, const glm::vec4& typeColour, const glm::vec4& logColour) : typeName(type), log(log), typeColour(typeColour), logColour(logColour) {}

	const char* GetTypeName() const { return typeName; }
	const char* GetLog() const { return log; }
	const glm::vec4& GetTypeColour() const { return typeColour; }
	const glm::vec4& GetLogColour() const { return logColour; }
private:
	const char* typeName;
	const char* log;
	const glm::vec4 typeColour;
	const glm::vec4 logColour;
};

static class Logger {
public:
	static void Log(const char* message) {
		const char* logType = "[Log]     ";
		entries.push_back(LogEntry(logType, message, glm::vec4(1.0f), glm::vec4(1.0f)));
		std::cout << logType << message << std::endl;
	}
	static void LogError(const char* message) {
		const char* logType = "[ERROR]   ";
		entries.push_back(LogEntry(logType, message, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
		std::cout << logType << message << std::endl;
	}
	static void LogWarning(const char* message) {
		const char* logType = "[Warning] ";
		entries.push_back(LogEntry(logType, message, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
		std::cout << logType << message << std::endl;
	}
	static void CustomLog(const char* logType, const char* message, const glm::vec4& typeColour, const glm::vec4& logColour) {
		entries.push_back(LogEntry(logType, message, typeColour, logColour));
		std::cout << logType << message << std::endl;
	}
	static void ClearLog() {
		entries.clear();
	}
	static const std::vector<LogEntry>& GetEntries() { return entries; }
private:
	static std::vector<LogEntry> entries;
};