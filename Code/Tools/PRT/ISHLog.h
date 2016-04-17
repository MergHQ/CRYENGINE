/*
	logging interface
*/

#pragma once


namespace NSH
{
	//!< logging interface (restricted flexibility since it is just used inside SH framework)
	struct ISHLog
	{
		virtual void Log(const char *cpLogText, ... ) = 0;					//!< log message
		virtual void LogError( const char *cpLogText, ... ) = 0;		//!< log error
		virtual void LogWarning( const char *cpLogText, ... ) = 0;	//!< log warning
		virtual void LogTime() = 0;																	//!< logs the current time in hours + minutes			
	};
}

