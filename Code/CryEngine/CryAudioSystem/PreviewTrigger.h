// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"

namespace CryAudio
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
class CTriggerConnection;

namespace Impl
{
struct ITriggerConnection;
struct ITriggerInfo;
} // namespace Impl

class CPreviewTrigger final : public Control
{
public:

	CPreviewTrigger(CPreviewTrigger const&) = delete;
	CPreviewTrigger(CPreviewTrigger&&) = delete;
	CPreviewTrigger& operator=(CPreviewTrigger const&) = delete;
	CPreviewTrigger& operator=(CPreviewTrigger&&) = delete;

	CPreviewTrigger();
	~CPreviewTrigger();

	void Execute(Impl::ITriggerInfo const& triggerInfo);
	void Stop();
	void Clear();

private:

	Impl::ITriggerConnection const* m_pConnection;
};
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
