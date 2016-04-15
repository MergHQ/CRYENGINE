/*
	logging implementation
*/

#pragma once

#include "stdafx.h"
#include "SHLog.h"
#include <PRT/PRTTypes.h>
#include <windows.h>


NSH::ISHLog& GetSHLog()
{
	return NSH::CSHLog::Instance();
}

NSH::CSHLog& NSH::CSHLog::Instance()
{
	static CSHLog inst;
	return inst;
}

NSH::CSHLog::CSHLog() : m_RecordedWarningsCount(0), m_RecordedErrorCount(0)
{
	//create files
	FILE *pErrorFile = fopen(scErrorFileName, "wb");
	FILE *pLogFile = fopen(scLogFileName, "wb");
//	assert(pErrorFile);
//	assert(pLogFile);
	if(pErrorFile)
		fclose(pErrorFile);
	if(pLogFile)
		fclose(pLogFile);
}

NSH::CSHLog::~CSHLog()
{ 
	if(m_RecordedWarningsCount > 0)
		printf("\n%d warnings in PRT engine recorded, see log file: %s\n", m_RecordedWarningsCount, scLogFileName);
	if(m_RecordedErrorCount > 0)
		printf("%d erors in PRT engine recorded, see log file: %s\n", m_RecordedErrorCount, scErrorFileName);
	printf("\n"); 
}

void NSH::CSHLog::Log(const char *cpLogText, ... )
{
	FILE *pLogFile = fopen(scLogFileName, "a");
//	assert(pLogFile);
	if(!pLogFile)
		return;
	char str[1024];
	va_list argptr;
	va_start(argptr,cpLogText);
	vsprintf(str,cpLogText,argptr);
	assert(strlen(str)<1023);					
	fprintf(pLogFile, str);
	va_end(argptr);
	fclose(pLogFile);
}

void NSH::CSHLog::LogError( const char *cpLogText, ... )
{
	FILE *pErrorFile = fopen(scErrorFileName, "a");
//	assert(pErrorFile);
	if(!pErrorFile)
		return;
	m_RecordedErrorCount++;
	LogTime(pErrorFile);
	fprintf(pErrorFile, "ERROR: ");
	printf("ERROR: ");
	char str[1024];
	va_list argptr;
	va_start(argptr,cpLogText);
	vsprintf(str,cpLogText,argptr);
	assert(strlen(str)<1023);					
	fprintf(pErrorFile, str);
	printf(str);
	if(std::string(str).find("\n") == std::string::npos)
	{
		fprintf(pErrorFile, "\n");
		printf("\n");
	}
	va_end(argptr);
	fclose(pErrorFile);
}

void NSH::CSHLog::LogWarning( const char *cpLogText, ... )
{
	FILE *pLogFile = fopen(scLogFileName, "a");
//	assert(pLogFile);
	if(!pLogFile)
		return;
	m_RecordedWarningsCount++;
	LogTime(pLogFile);
	fprintf(pLogFile, "WARNING: ");
	char str[1024];
	va_list argptr;
	va_start(argptr,cpLogText);
	vsprintf(str,cpLogText,argptr);
	assert(strlen(str)<1023);					
	fprintf(pLogFile, str);
	if(std::string(str).find("\n") == std::string::npos)
		fprintf(pLogFile, "\n");
	va_end(argptr);
	fclose(pLogFile);
}

void NSH::CSHLog::LogTime(FILE *pFile)
{
	assert(pFile);
	if(!pFile)
		return;
	SYSTEMTIME time;
	GetLocalTime(&time);
	if(time.wMinute < 10)
		fprintf(pFile,"%d.0%d: ",time.wHour,time.wMinute);	
	else
		fprintf(pFile,"%d.%d: ",time.wHour,time.wMinute);	
}

void NSH::CSHLog::LogTime()
{
	FILE *pLogFile = fopen(scLogFileName, "a");
	if(!pLogFile)
		return; 
	LogTime(pLogFile);
	fclose(pLogFile);
}
