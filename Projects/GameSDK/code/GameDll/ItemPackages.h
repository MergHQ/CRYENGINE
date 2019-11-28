// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CItemPackages
{
public:

	typedef std::vector<IEntityClass*> TSetup;

	struct SPackage
	{
		ItemString m_displayName;
		IEntityClass* m_pItemClass;
		TSetup m_setup;
	};

	typedef std::vector<SPackage> TPackages;

public:
	void Load();
	const char* GetFullItemName(const CItem* pItem) const;

private:
	TPackages m_packages;
};
