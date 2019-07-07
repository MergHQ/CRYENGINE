// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IExportManager.h>

class COBJExporter : public IExporter
{
public:
	virtual const char* GetExtension() const;
	virtual const char* GetShortDescription() const;
	virtual bool        ExportToFile(const char* filename, const SExportData* pExportData);
	virtual bool        ImportFromFile(const char* filename, SExportData* pData) { return false; }
	virtual void        Release();

private:
	const char* TrimFloat(float fValue) const;
	string      MakeRelativePath(const char* pMainFileName, const char* pFileName) const;
};
