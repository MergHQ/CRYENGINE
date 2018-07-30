// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#pragma once

struct IFilesGroupProvider
{
	virtual ~IFilesGroupProvider() {}

	//! Returns paths to the files the object owns. The paths are relative to the assets root directory.
	virtual std::vector<string> GetFiles() const = 0;

	virtual const string& GetMainFile() const = 0;

	virtual const string& GetName() const = 0;

	//! Updates the file group to reflect possible change in the list of files.
	virtual void Update() {}
};
