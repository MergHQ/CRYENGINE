// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>

namespace CryAudio
{
class CATLStandaloneFile;

namespace Impl
{
namespace Fmod
{
class CBaseObject;

class CBaseStandaloneFile : public IStandaloneFile
{
public:

	CBaseStandaloneFile() = delete;
	CBaseStandaloneFile(CBaseStandaloneFile const&) = delete;
	CBaseStandaloneFile(CBaseStandaloneFile&&) = delete;
	CBaseStandaloneFile& operator=(CBaseStandaloneFile const&) = delete;
	CBaseStandaloneFile& operator=(CBaseStandaloneFile&&) = delete;

	explicit CBaseStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile);
	virtual ~CBaseStandaloneFile() override;

	virtual void StartLoading() = 0;
	virtual bool IsReady() = 0;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) = 0;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) = 0;
	virtual void Stop() = 0;

	void         ReportFileStarted();
	void         ReportFileFinished();

	CATLStandaloneFile&                m_atlStandaloneFile;
	CryFixedStringT<MaxFilePathLength> m_fileName;
	CBaseObject*                       m_pObject = nullptr;
	static FMOD::System*               s_pLowLevelSystem;

};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
