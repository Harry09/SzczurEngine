#pragma once

#include "Szczur/Config.hpp"
#include "Szczur/Utility/Convert/GLMStreams.hpp"

#include <fstream>
#ifdef DEBUG
#	include <iostream>
#endif

namespace rat
{

class Logger
{
public:

	///
	Logger();

	///
	Logger(const Logger&) = delete;

	///
	Logger& operator = (const Logger&) = delete;

	///
	Logger(Logger&&) = delete;

	///
	Logger& operator = (Logger&&) = delete;

	///
	~Logger();

	///
	template <typename... Us>
	void log(const char* file, int line, const char* levelName, Us&&... args);

	///
	void logException(const char* file, int line, const std::exception& exception, int level = 0);

private:

	///
	void _formatTime(const char* format);

	char _buffer[128];
	char _logPath[64];
	char _timeBuffer[16];
	std::ofstream _logFile;

};

inline Logger* logger = nullptr;

}

#include "Logger.tpp"

#if defined(COMPILER_MSVC)
#    include <cstring>
#    define __FILENAME__ (std::strrchr(__FILE__, DIRECTORY_SEPARATOR_CHAR) ? std::strrchr(__FILE__, DIRECTORY_SEPARATOR_CHAR) + 1 : __FILE__)
#else
#    define __FILENAME__ __FILE__
#endif

#define LOG_INFO(...) { rat::logger->log(__FILENAME__, __LINE__, "INFO", __VA_ARGS__); }
#define LOG_WARNING(...) { rat::logger->log(__FILENAME__, __LINE__, "WARNING", __VA_ARGS__); }
#define LOG_ERROR(...) { rat::logger->log(__FILENAME__, __LINE__, "ERROR", __VA_ARGS__); }
#define LOG_INFO_IF(condition, ...) { if (condition) LOG_INFO(__VA_ARGS__) }
#define LOG_WARNING_IF(condition, ...) { if (condition) LOG_WARNING(__VA_ARGS__) }
#define LOG_ERROR_IF(condition, ...) { if (condition) LOG_ERROR(__VA_ARGS__) }
#define LOG_INFO_IF_CX(condition, ...) { if constexpr (condition) LOG_INFO(__VA_ARGS__) }
#define LOG_WARNING_IF_CX(condition, ...) { if constexpr (condition) LOG_WARNING(__VA_ARGS__) }
#define LOG_ERROR_IF_CX(condition, ...) { if constexpr (condition) LOG_ERROR(__VA_ARGS__) }
#define LOG_EXCEPTION(exception) { rat::logger->logException(__FILENAME__, __LINE__, exception); }
