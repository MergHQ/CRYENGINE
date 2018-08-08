// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

#include "Common.h"

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CEvent;
class CTrigger;
class CParameter;
class CSwitchState;

class CObjectBase : public IObject
{
public:

	CObjectBase();
	virtual ~CObjectBase() override;

	CObjectBase(CObjectBase const&) = delete;
	CObjectBase(CObjectBase&&) = delete;
	CObjectBase& operator=(CObjectBase const&) = delete;
	CObjectBase& operator=(CObjectBase&&) = delete;

	// CryAudio::Impl::IObject
	virtual void           Update() override;
	virtual void           SetTransformation(CObjectTransformation const& transformation) override;
	virtual ERequestStatus ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent) override;
	virtual void           StopAllTriggers() override;
	virtual ERequestStatus PlayFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus StopFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus SetName(char const* const szName) override;
	// ~CryAudio::Impl::IObject

	bool StartEvent(CTrigger const* const pTrigger, CEvent* const pEvent);
	void StopEvent(uint32 const triggerId);
	void PauseEvent(uint32 const triggerId);
	void ResumeEvent(uint32 const triggerId);

	void RemoveEvent(CEvent* const pEvent);

protected:

	std::vector<CEvent*> m_events;

	STransformation      m_transformation;
	CriAtomEx3dSourceHn  m_p3dSource;
	CriAtomExPlayerHn    m_pPlayer;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
