// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IConvertor.h"  // IConvertor
#include <CryCore/StlUtils.h>    // stl::less_stricmp

struct IResourceCompiler;

class CImageConvertor
	: public IConvertor
{
public:
	// Maps aliases of presets to real names of presets
	typedef std::map<string, string, stl::less_stricmp<string> > PresetAliases;

public:
	explicit CImageConvertor(IResourceCompiler* pRC);
	~CImageConvertor();

	// interface IConvertor ----------------------------------------------------
	virtual void Release();
	virtual void Init(const ConvertorInitContext& context);
	virtual ICompiler* CreateCompiler();
	virtual bool SupportsMultithreading() const;
	virtual const char* GetExt(int index) const;
	virtual bool IsCacheable() const override { return true; }
	// -------------------------------------------------------------------------

private:
	PresetAliases m_presetAliases;
};
