// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

struct IFilesGroupController
{
	virtual ~IFilesGroupController() {}

	//! Returns paths to the files the object owns. The paths are relative to the assets root directory.
	//! \param includeGeneratedFile specifies if generated file (like thumbnail) needs to be included.
	virtual std::vector<string> GetFiles(bool includeGeneratedFile = true) const = 0;

	virtual const string& GetMainFile() const = 0;

	virtual const string& GetName() const = 0;

	//! Returns the name of generated file (like thumbnail). If there is no such file returns empty string.
	virtual string GetGeneratedFile() const { return ""; }

	//! Updates the file group to reflect possible change in the list of files.
	//! Note: this method can be called even for deleted or not existing in memory item (assets, layers).
	//! So certain measures need to be taken to make sure this case is properly handled.
	virtual void Update() {}
};
