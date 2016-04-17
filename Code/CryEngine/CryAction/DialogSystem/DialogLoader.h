// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   DialogLoader.h
//  Version:     v1.00
//  Created:     07/07/2006 by AlexL
//  Compilers:   Visual Studio.NET
//  Description: Dialog Loader
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __DIALOGLOADER_H__
#define __DIALOGLOADER_H__

#pragma once

#include "DialogScript.h"

class CDialogSystem;

class CDialogLoader
{
public:
	CDialogLoader(CDialogSystem* pDS);
	virtual ~CDialogLoader();

	// Loads all DialogScripts below a given path
	bool LoadScriptsFromPath(const string& path, TDialogScriptMap& outScripts);

	// Loads a single DialogScript
	bool LoadScript(const string& fileName, TDialogScriptMap& outScripts);

protected:

	// some intermediate data structure
	struct IMScriptLine
	{
		IMScriptLine() { Reset(); }
		void Reset()
		{
			audioID = INVALID_AUDIO_CONTROL_ID;
			actor = anim = facial = lookat = "";
			delay = facialFadeTime = 0.0f;
			facialWeight = 0.5f;
		}

		bool IsValid() const { return (actor != 0 && *actor != 0); }

		const char*    actor;
		//const char* sound;
		AudioControlId audioID;
		const char*    anim;
		const char*    lookat;
		float          delay;
		string         facial;
		float          facialWeight;
		float          facialFadeTime;
	};

	struct IMScript
	{
		IMScript() { Reset(); }
		void Reset()         { name = 0; lines.resize(0); }
		bool IsValid() const { return (name != 0 && *name != 0 && lines.size() > 0); }
		const char*               name;
		std::vector<IMScriptLine> lines;
	};

	// returns the numer of successfully loaded scripts
	int LoadFromTable(XmlNodeRef tableNode, const string& fileName, TDialogScriptMap& outScriptMap);

	// preprocessing stuff
	bool ProcessScript(IMScript& theScript, const string& groupName, TDialogScriptMap& outScriptMap);

	// get actor from string [1-based]
	bool GetActor(const char* actor, int& outID);
	bool GetLookAtActor(const char* actor, int& outID, bool& outSticky);

protected:

	CDialogSystem* m_pDS;
};

#endif
