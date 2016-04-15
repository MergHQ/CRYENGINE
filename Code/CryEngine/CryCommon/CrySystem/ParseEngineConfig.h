// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$

   -------------------------------------------------------------------------
   History:
   - 17:02:2009  Extracted from Launcher/Main.cpp by Alex McCarthy
   - 2013-01-07  Replaced with class version from various Main.cpp files,
              now read <game>/game.cfg as well [J Scott Peter]

*************************************************************************/

#ifndef __PARSEENGINECONFIG_H__
#define __PARSEENGINECONFIG_H__

#define ENGINE_CFG_FILE              "system.cfg"
#define GAME_CFG_FILE                "game.cfg"
#define PROJECT_CFG_FILE             "project.cfg"

#define CONFIG_KEY_FOR_GAMEDLL       "sys_dll_game"
#define CONFIG_KEY_FOR_GAMEFOLDER    "sys_game_folder"
#define CONFIG_KEY_FOR_DATAFOLDER    "sys_data_folder"
#define CONFIG_KEY_FOR_ENGINEVERSION "engine_version"

#define DEFAULT_GAMEDLL              "CryGameZero.dll"
#define DEFAULT_GAMEFOLDER           "Game"
#define DEFAULT_DATAFOLDER           "Data"

//////////////////////////////////////////////////////////////////////////
class CEngineConfig
{
public:
	string m_gameDLL;
	string m_dataFolder;
	string m_gameFolder;

public:

	CEngineConfig()
		: m_gameDLL(DEFAULT_GAMEDLL)
		, m_gameFolder(DEFAULT_GAMEFOLDER)
		, m_dataFolder(DEFAULT_DATAFOLDER)
	{
		ParseConfig(ENGINE_CFG_FILE);
		if (!m_gameFolder.empty())
			ParseConfig(m_gameFolder + CRY_NATIVE_PATH_SEPSTR GAME_CFG_FILE);
	}

	//////////////////////////////////////////////////////////////////////////
	bool ParseConfig(const char* filename)
	{
		FILE* file = fopen(filename, "rb");
		if (!file)
		{
			int filenameLength = strlen(filename);
			char* filenameLwr = (char*) malloc(filenameLength + 1);
			for (int i = 0; i <= filenameLength; i++)
			{
				filenameLwr[i] = tolower(filename[i]);
			}
			file = fopen(filenameLwr, "rb");
			free(filenameLwr);
			if (!file)
			{
				return false;
			}
		}

		fseek(file, 0, SEEK_END);
		int nLen = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* sAllText = new char[nLen + 16];

		fread(sAllText, 1, nLen, file);

		sAllText[nLen] = '\0';
		sAllText[nLen + 1] = '\0';

		string strGroup;      // current group e.g. "[General]"

		char* strLast = sAllText + nLen;
		char* str = sAllText;
		while (str < strLast)
		{
			char* s = str;
			while (str < strLast && *str != '\n' && *str != '\r')
				str++;
			*str = '\0';
			str++;
			while (str < strLast && (*str == '\n' || *str == '\r'))
				str++;

			string strLine = s;

			// detect groups e.g. "[General]"   should set strGroup="General"
			{
				string strTrimmedLine(RemoveWhiteSpaces(strLine));
				size_t size = strTrimmedLine.size();

				if (size >= 3)
					if (strTrimmedLine[0] == '[' && strTrimmedLine[size - 1] == ']')   // currently no comments are allowed to be behind groups
					{
						strGroup = &strTrimmedLine[1];
						strGroup.resize(size - 2);                                // remove [ and ]
						continue;                                                 // next line
					}
			}

			// skip comments
			if (0 < strLine.find("--"))
			{
				// extract key
				string::size_type posEq(strLine.find("=", 0));
				if (string::npos != posEq)
				{
					string stemp(strLine, 0, posEq);
					string strKey(RemoveWhiteSpaces(stemp));

					//				if (!strKey.empty())
					{
						// extract value
						string::size_type posValueStart(strLine.find("\"", posEq + 1) + 1);
						// string::size_type posValueEnd( strLine.find( "\"", posValueStart ) );
						string::size_type posValueEnd(strLine.rfind('\"'));

						string strValue;

						if (string::npos != posValueStart && string::npos != posValueEnd)
							strValue = string(strLine, posValueStart, posValueEnd - posValueStart);
						else
						{
							string strTmp(strLine, posEq + 1, strLine.size() - (posEq + 1));
							strValue = RemoveWhiteSpaces(strTmp);
						}

						{
							string strTemp;
							strTemp.reserve(strValue.length() + 1);
							// replace '\\\\' with '\\' and '\\\"' with '\"'
							for (string::const_iterator iter = strValue.begin(); iter != strValue.end(); ++iter)
							{
								if (*iter == '\\')
								{
									++iter;
									if (iter == strValue.end())
										;
									else if (*iter == '\\')
										strTemp += '\\';
									else if (*iter == '\"')
										strTemp += '\"';
								}
								else
									strTemp += *iter;
							}
							strValue.swap(strTemp);

							//						m_pSystem->GetILog()->Log("Setting %s to %s",strKey.c_str(),strValue.c_str());
							OnLoadConfigurationEntry(strKey, strValue, strGroup);
						}
					}
				}
			} //--
		}
		delete[] sAllText;
		fclose(file);

		return true;
	}

protected:

	virtual void OnLoadConfigurationEntry(const string& strKey, const string& strValue, const string& strGroup)
	{
		if (strKey.compareNoCase(CONFIG_KEY_FOR_GAMEDLL) == 0)
		{
			m_gameDLL = strValue;
		}
		if (strKey.compareNoCase(CONFIG_KEY_FOR_DATAFOLDER) == 0)
		{
			m_dataFolder = strValue;
		}
		if (strKey.compareNoCase(CONFIG_KEY_FOR_GAMEFOLDER) == 0)
		{
			m_gameFolder = strValue;
		}
	}

	string RemoveWhiteSpaces(string& s)
	{
		s.Trim();
		return s;
	}
};

class CProjectConfig : public CEngineConfig
{
public:
	string m_engineVersion;

	CProjectConfig()
	{
		ParseConfig(PROJECT_CFG_FILE);
	}

	void OnLoadConfigurationEntry(const string& strKey, const string& strValue, const string& strGroup) override
	{
		if (strKey.compareNoCase(CONFIG_KEY_FOR_ENGINEVERSION) == 0)
		{
			m_engineVersion = strValue;
		}
		else if (strKey.compareNoCase(CONFIG_KEY_FOR_GAMEDLL) == 0)
		{
			m_gameDLL = strValue;
		}
		else if (strKey.compareNoCase(CONFIG_KEY_FOR_GAMEFOLDER) == 0)
		{
			m_gameFolder = strValue;
		}
	}
};

#endif //__PARSEENGINECONFIG_H__
