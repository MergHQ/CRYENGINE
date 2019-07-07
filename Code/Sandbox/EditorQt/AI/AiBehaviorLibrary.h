// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AiBehavior.h"
#include "Objects/EntityScript.h"
#include <vector>
#include <map>

/*!
 * CAIBehaviorLibrary is collection of global AI behaviors.
 */
class CAIBehaviorLibrary
{
public:
	CAIBehaviorLibrary();
	~CAIBehaviorLibrary() {}

	//! Add new behavior to the library.
	void         AddBehavior(CAIBehavior* behavior);
	//! Remove behavior from the library.
	void         RemoveBehavior(CAIBehavior* behavior);

	CAIBehavior* FindBehavior(const string& name) const;

	//! Clear all behaviors from library.
	void ClearBehaviors();

	//! Get all stored behaviors as a vector.
	void GetBehaviors(std::vector<CAIBehaviorPtr>& behaviors);

	//! Load all behaviors from given path and add them to library.
	void LoadBehaviors(const string& path);

	//! Reload behavior scripts.
	void ReloadScripts();

	//! Get all available characters in system.
	void GetCharacters(std::vector<CAICharacterPtr>& characters);

	//! Add new behavior to the library.
	void AddCharacter(CAICharacter* chr);
	//! Remove behavior from the library.
	void RemoveCharacter(CAICharacter* chr);

	// Finds specified character.
	CAICharacter* FindCharacter(const string& name) const;

private:
	void LoadCharacters();

	std::map<string, TSmartPtr<CAIBehavior>>  m_behaviors;
	std::map<string, TSmartPtr<CAICharacter>> m_characters;
	string m_scriptsPath;
};
