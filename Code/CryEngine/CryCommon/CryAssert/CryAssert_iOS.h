// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:
   Assert dialog box for Mac OS X

   -------------------------------------------------------------------------
   History:
   - 03:09:2013: Created by Leander Beernaert

*************************************************************************/

#if defined(USE_CRY_ASSERT) && CRY_PLATFORM_IOS

static char gs_szMessage[MAX_PATH];

void CryAssertTrace(const char* szFormat, ...)
{
	if (gEnv == 0)
	{
		return;
	}

	if (!gEnv->ignoreAllAsserts || gEnv->bTesting)
	{
		if (szFormat == NULL || szFormat[0] == '\0')
		{
			gs_szMessage[0] = '\0';
		}
		else
		{
			va_list args;
			va_start(args, szFormat);
			cry_vsprintf(gs_szMessage, szFormat, args);
			va_end(args);
		}
	}
}

bool CryAssert(const char* szCondition, const char* szFile, unsigned int line, bool* pIgnore)
{
	if (!gEnv) return false;

	static const int max_len = 4096;
	static char gs_command_str[4096];

	gEnv->pSystem->OnAssert(szCondition, gs_szMessage, szFile, line);

	size_t file_len = strlen(szFile);

	if (!gEnv->bUnattendedMode && !gEnv->ignoreAllAsserts && !gEnv->noAssertDialog)
	{
		CryLogAlways("!!ASSERT!!\n\tCondition: %s\n\tMessage  : %s\n\tFile     : %s\n\tLine     : %d", szCondition, gs_szMessage, szFile, line);
	}

#if defined(eCryModule)
	const char* szModuleName = GetCryModuleName(eCryModule);
#else
	const char* szModuleName = "Undefined";
#endif

	gEnv->pLog->LogError("Assertion Failed! %s:%d reason:%s Module:%s", szFile, line, gs_szMessage[0] ? gs_szMessage : "<empty>", szModuleName);

	return false;
}

#endif
