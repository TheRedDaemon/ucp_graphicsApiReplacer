#pragma once

namespace UCPtoOpenGL
{
	// normal enum, to allow easier transform to int 
	enum LogLevel
	{
		LOG_FATAL		=		-3,
		LOG_ERROR		=		-2,
		LOG_WARNING =		-1,
		LOG_INFO		=		0,
		LOG_DEBUG		=		1,
	};

	void Log(LogLevel level, std::string message);
}