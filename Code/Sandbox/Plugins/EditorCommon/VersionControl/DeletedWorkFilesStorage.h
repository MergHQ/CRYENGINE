// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "CrySandbox/CrySignal.h"

//! This class stores deleted work files in a file
//! This list is used by VCS because otherwise it has no way of figuring out if they were deleted. 
//! Especially if this happened while VCS was offline.
class EDITOR_COMMON_API CDeletedWorkFilesStorage
{
public:
	static CDeletedWorkFilesStorage& GetInstance()
	{
		static CDeletedWorkFilesStorage storage;
		return storage;
	}

	void Add(const string& file);

	void Remove(const string& file);

	void Save();

	void Load();

	bool Contains(const string& file) const;

	int  GetSize() const;

	const string& GetFile(int index) const;

	CCrySignal<void()> signalSaved;
};
