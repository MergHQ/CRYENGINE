// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntityData.h"
#include <SharedAudioData.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CEvent;
class CParameter;
class CSwitchState;
class CEnvironment;
class CStandaloneFileBase;

enum class EObjectFlags : EnumFlagsType
{
	None                    = 0,
	MovingOrDecaying        = BIT(0),
	TrackAbsoluteVelocity   = BIT(1),
	TrackVelocityForDoppler = BIT(2),
	UpdateVirtualStates     = BIT(3),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EObjectFlags);

class CBaseObject : public IObject
{
public:

	CBaseObject();
	virtual ~CBaseObject() override;

	void RemoveEvent(CEvent* const pEvent);
	void RemoveParameter(CParameter const* const pParameter);
	void RemoveSwitch(CSwitchState const* const pSwitch);
	void RemoveEnvironment(CEnvironment const* const pEnvironment);
	void RemoveFile(CStandaloneFileBase const* const pStandaloneFile);

	// CryAudio::Impl::IObject
	virtual void                         Update(float const deltaTime) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override {}
	virtual CObjectTransformation const& GetTransformation() const override                                      { return m_transformation; }
	virtual ERequestStatus               ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent) override;
	virtual void                         StopAllTriggers() override;
	virtual ERequestStatus               PlayFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               StopFile(IStandaloneFile* const pIStandaloneFile) override;
	virtual ERequestStatus               SetName(char const* const szName) override;

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override {}
	// ~CryAudio::Impl::IObject

protected:

	void StopEvent(uint32 const id);
	bool SetEvent(CEvent* const pEvent);

	std::vector<CEvent*>                        m_events;
	std::vector<CStandaloneFileBase*>           m_files;
	std::map<CParameter const* const, float>    m_parameters;
	std::map<uint32 const, CSwitchState const*> m_switches;
	std::map<CEnvironment const* const, float>  m_environments;

	std::vector<CEvent*>                        m_pendingEvents;
	std::vector<CStandaloneFileBase*>           m_pendingFiles;

	FMOD_3D_ATTRIBUTES                          m_attributes;
	float                 m_occlusion = 0.0f;

	CObjectTransformation m_transformation;

public:

	static FMOD::Studio::System* s_pSystem;
};

using Objects = std::vector<CBaseObject*>;

class CObject final : public CBaseObject, public CPoolObject<CObject, stl::PSyncNone>
{
public:

	CObject() = delete;

	CObject(CObjectTransformation const& transformation);
	virtual ~CObject() override;

	// CryAudio::Impl::IObject
	virtual void Update(float const deltaTime) override;
	virtual void SetTransformation(CObjectTransformation const& transformation) override;
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter) override;
	// ~CryAudio::Impl::IObject

private:

	void Set3DAttributes();
	void UpdateVelocities(float const deltaTime);
	void SetParameterById(uint32 const parameterId, float const value);

	EObjectFlags m_flags;
	float        m_previousAbsoluteVelocity;
	Vec3         m_position;
	Vec3         m_previousPosition;
	Vec3         m_velocity;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	std::map<char const* const, float> m_parameterInfo;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};

class CGlobalObject final : public CBaseObject
{
public:

	CGlobalObject(Objects const& objects)
		: m_objects(objects)
	{}

	// CryAudio::Impl::IObject
	virtual void SetEnvironment(IEnvironment const* const pIEnvironment, float const amount) override;
	virtual void SetParameter(IParameter const* const pIParameter, float const value) override;
	virtual void SetSwitchState(ISwitchState const* const pISwitchState) override;
	virtual void SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual void SetOcclusionType(EOcclusionType const occlusionType) override;
	virtual void ToggleFunctionality(EObjectFunctionality const type, bool const enable) override;
	// ~CryAudio::Impl::IObject

private:

	Objects const& m_objects;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
