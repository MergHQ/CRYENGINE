// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
class CATLAudioObject;
class CATLStandaloneFile;
class CATLEvent;
class CAudioRequest;

class CObjectManager final
{
public:

	using ConstructedObjects = std::vector<CATLAudioObject*>;

	CObjectManager() = default;
	~CObjectManager();

	CObjectManager(CObjectManager const&) = delete;
	CObjectManager(CObjectManager&&) = delete;
	CObjectManager& operator=(CObjectManager const&) = delete;
	CObjectManager& operator=(CObjectManager&&) = delete;

	void            Initialize(uint32 const poolSize);
	void            Terminate();
	void            OnAfterImplChanged();
	void            ReleaseImplData();
	void            Update(float const deltaTime);
	void            RegisterObject(CATLAudioObject* const pObject);

	void            ReportStartedEvent(CATLEvent* const pEvent);
	void            ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess);
	void            GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request);
	void            ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile);
	void            ReleasePendingRays();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	size_t                    GetNumAudioObjects() const;
	size_t                    GetNumActiveAudioObjects() const;
	ConstructedObjects const& GetObjects() const { return m_constructedObjects; }
	void                      DrawPerObjectDebugInfo(IRenderAuxGeom& auxGeom) const;
	void                      DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY) const;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	ConstructedObjects m_constructedObjects;
};
} // namespace CryAudio
