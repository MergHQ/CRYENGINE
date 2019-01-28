// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Entity.h"

namespace CryAudio
{
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
	void Clear();
	bool HasConnection() const { return m_pConnection != nullptr; }

private:

	Impl::ITriggerConnection* m_pConnection;
};
}      // namespace CryAudio
#endif // CRY_AUDIO_USE_PRODUCTION_CODE