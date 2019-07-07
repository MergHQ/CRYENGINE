// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// These are helper classes for containing the data from the generic overwrite dialog.

// Small helper class.
// Hint: have one for files and other for directories.
// Hint: used a CUserOptionsReferenceCountHelper to automatically control the reference counts
// of any CUserOptions variable: useful for recursion when you don't want to use
// only static variables. See example in FileUtill.cpp, function CopyTree.
class CUserOptions
{
public:
	enum EOption
	{
		ENotSet,
		EYes    = IDYES,
		ENo     = IDNO,
		ECancel = IDCANCEL,
	};

	class CUserOptionsReferenceCountHelper
	{
	public:
		CUserOptionsReferenceCountHelper(CUserOptions& roUserOptions);
		~CUserOptionsReferenceCountHelper();
	protected:
		CUserOptions& m_roReferencedUserOptionsObject;
	};
public:
	CUserOptions();

	bool IsOptionValid();

	int  GetOption();

	bool IsOptionToAll();

	void SetOption(int nNewOption, bool boToAll);

	int  DecRef();
	int  IncRef();
protected:
	int  m_nCurrentOption;
	bool m_boToAll;
	int  m_nNumberOfReferences;
};
