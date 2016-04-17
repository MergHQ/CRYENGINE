/*
	logging declaration
*/

#pragma once

#include <PRT/ISHLog.h>
#include <stdarg.h>
#include <stdio.h>


namespace NSH
{
	static const char* scLogFileName = "prtlog.txt";
	static const char* scErrorFileName = "prterror.txt";

	//!< logging implementation as singleton
	//!< hosts two files(prtlog.txt, prterror.txt)
	//!< always adds time
	class CSHLog : public ISHLog
	{
	public:
		virtual void Log(const char *cpLogText, ... );					//!< log message
		virtual void LogError( const char *cpLogText, ... );		//!< log error
		virtual void LogWarning( const char *cpLogText, ... );	//!< log warning
		static CSHLog& Instance();															//!< retrieve logging instance
		
		virtual ~CSHLog();				//!< destructor freeing files
		virtual void LogTime();		//!< logs the time for the log file

	private:
		unsigned int m_RecordedWarningsCount;	//!< number of recorded warnings
		unsigned int m_RecordedErrorCount;		//!< number of recorded warnings

		//!< singleton stuff
		CSHLog();
		CSHLog(const CSHLog&);
		CSHLog& operator= (const CSHLog&);

		void LogTime(FILE *pFile);		//!< logs the time for any file
	};

}
