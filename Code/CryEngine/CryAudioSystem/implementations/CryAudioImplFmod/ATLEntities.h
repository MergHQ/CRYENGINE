// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioEvent.h"
#include "AudioObject.h"
#include "GlobalData.h"

namespace CryAudio
{
class CATLStandaloneFile;

namespace Impl
{
namespace Fmod
{
class CListener final : public IListener
{
public:

	CListener() = delete;
	CListener(CListener const&) = delete;
	CListener(CListener&&) = delete;
	CListener& operator=(CListener const&) = delete;
	CListener& operator=(CListener&&) = delete;

	explicit CListener(CObjectTransformation const& transformation, int const id);
	virtual ~CListener() override = default;

	ILINE int                 GetId() const     { return m_id; }
	ILINE FMOD_3D_ATTRIBUTES& Get3DAttributes() { return m_attributes; }

	// CryAudio::Impl::IListener
	virtual void                         Update(float const deltaTime) override;
	virtual void                         SetName(char const* const szName) override;
	virtual void                         SetTransformation(CObjectTransformation const& transformation) override;
	virtual CObjectTransformation const& GetTransformation() const override { return m_transformation; }
	// ~CryAudio::Impl::IListener

	Vec3 const& GetPosition() const { return m_position; }
	Vec3 const& GetVelocity() const { return m_velocity; }

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	static FMOD::Studio::System* s_pSystem;

private:

	void SetVelocity();

	int                   m_id;
	bool                  m_isMovingOrDecaying;
	Vec3                  m_velocity;
	Vec3                  m_position;
	Vec3                  m_previousPosition;
	CObjectTransformation m_transformation;
	FMOD_3D_ATTRIBUTES    m_attributes;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxObjectNameLength> m_name;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};

enum class EEventType : EnumFlagsType
{
	None,
	Start,
	Stop,
	Pause,
	Resume,
};

class CTrigger final : public ITrigger
{
public:

	explicit CTrigger(
		uint32 const id,
		EEventType const eventType,
		FMOD::Studio::EventDescription* const pEventDescription,
		FMOD_GUID const guid,
		bool const hasProgrammerSound = false,
		char const* const szKey = "")
		: m_id(id)
		, m_eventType(eventType)
		, m_pEventDescription(pEventDescription)
		, m_guid(guid)
		, m_hasProgrammerSound(hasProgrammerSound)
		, m_key(szKey)
	{}

	virtual ~CTrigger() override = default;

	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	// CryAudio::Impl::ITrigger
	virtual ERequestStatus Load()  const override                            { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                           { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override { return ERequestStatus::Success; }
	// ~CryAudio::Impl::ITrigger

	uint32                                       GetId() const               { return m_id; }
	EEventType                                   GetEventType() const        { return m_eventType; }
	FMOD::Studio::EventDescription*              GetEventDescription() const { return m_pEventDescription; }
	FMOD_GUID                                    GetGuid() const             { return m_guid; }
	bool                                         HasProgrammerSound() const  { return m_hasProgrammerSound; }
	CryFixedStringT<MaxControlNameLength> const& GetKey() const              { return m_key; }

private:

	uint32 const                                m_id;
	EEventType const                            m_eventType;
	FMOD::Studio::EventDescription* const       m_pEventDescription;
	FMOD_GUID const                             m_guid;
	bool const                                  m_hasProgrammerSound;
	CryFixedStringT<MaxControlNameLength> const m_key;
};

enum class EParameterType : EnumFlagsType
{
	None,
	Parameter,
	VCA,
};

class CParameter : public IParameter
{
public:

	explicit CParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName,
		EParameterType const type)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
		, m_type(type)
	{}

	virtual ~CParameter() override = default;

	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter&                                  operator=(CParameter const&) = delete;
	CParameter&                                  operator=(CParameter&&) = delete;

	uint32                                       GetId() const              { return m_id; }
	float                                        GetValueMultiplier() const { return m_multiplier; }
	float                                        GetValueShift() const      { return m_shift; }
	EParameterType                               GetType() const            { return m_type; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const            { return m_name; }

private:

	uint32 const                                m_id;
	float const                                 m_multiplier;
	float const                                 m_shift;
	EParameterType const                        m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
};

class CVcaParameter final : public CParameter
{
public:

	explicit CVcaParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName,
		FMOD::Studio::VCA* const vca)
		: CParameter(id, multiplier, shift, szName, EParameterType::VCA)
		, m_vca(vca)
	{}

	FMOD::Studio::VCA* GetVca() const { return m_vca; }

private:

	FMOD::Studio::VCA* const m_vca;
};

enum class EStateType : EnumFlagsType
{
	None,
	State,
	VCA,
};

class CSwitchState : public ISwitchState
{
public:

	explicit CSwitchState(
		uint32 const id,
		float const value,
		char const* const szName,
		EStateType const type)
		: m_id(id)
		, m_value(value)
		, m_name(szName)
		, m_type(type)
	{}

	virtual ~CSwitchState() override = default;

	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState&                                operator=(CSwitchState const&) = delete;
	CSwitchState&                                operator=(CSwitchState&&) = delete;

	uint32                                       GetId() const    { return m_id; }
	float                                        GetValue() const { return m_value; }
	EStateType                                   GetType() const  { return m_type; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const  { return m_name; }

private:

	uint32 const                                m_id;
	float const                                 m_value;
	EStateType const                            m_type;
	CryFixedStringT<MaxControlNameLength> const m_name;
};

class CVcaState final : public CSwitchState
{
public:

	explicit CVcaState(
		uint32 const id,
		float const value,
		char const* const szName,
		FMOD::Studio::VCA* const vca)
		: CSwitchState(id, value, szName, EStateType::VCA)
		, m_vca(vca)
	{}

	FMOD::Studio::VCA* GetVca() const { return m_vca; }

private:

	FMOD::Studio::VCA* const m_vca;
};

enum class EEnvironmentType : EnumFlagsType
{
	None,
	Bus,
	Parameter,
};

class CEnvironment : public IEnvironment
{
public:

	explicit CEnvironment(EEnvironmentType const type)
		: m_type(type)
	{}

	virtual ~CEnvironment() override = default;

	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment&    operator=(CEnvironment const&) = delete;
	CEnvironment&    operator=(CEnvironment&&) = delete;

	EEnvironmentType GetType() const { return m_type; }

private:

	EEnvironmentType const m_type;
};

class CEnvironmentBus final : public CEnvironment
{
public:

	explicit CEnvironmentBus(
		FMOD::Studio::EventDescription* const pEventDescription,
		FMOD::Studio::Bus* const pBus)
		: CEnvironment(EEnvironmentType::Bus)
		, m_pEventDescription(pEventDescription)
		, m_pBus(pBus)
	{}

	virtual ~CEnvironmentBus() override = default;

	FMOD::Studio::EventDescription* GetEventDescription() const { return m_pEventDescription; }
	FMOD::Studio::Bus*              GetBus() const              { return m_pBus; }

private:

	FMOD::Studio::EventDescription* const m_pEventDescription;
	FMOD::Studio::Bus* const              m_pBus;
};

class CEnvironmentParameter final : public CEnvironment
{
public:

	explicit CEnvironmentParameter(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName)
		: CEnvironment(EEnvironmentType::Parameter)
		, m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}

	virtual ~CEnvironmentParameter() override = default;

	uint32                                       GetId() const              { return m_id; }
	float                                        GetValueMultiplier() const { return m_multiplier; }
	float                                        GetValueShift() const      { return m_shift; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const            { return m_name; }

private:

	uint32 const m_id;
	float const  m_multiplier;
	float const  m_shift;
	CryFixedStringT<MaxControlNameLength> const m_name;
};

class CFile final : public IFile
{
public:

	CFile() = default;
	virtual ~CFile() override = default;

	CFile(CFile const&) = delete;
	CFile(CFile&&) = delete;
	CFile& operator=(CFile const&) = delete;
	CFile& operator=(CFile&&) = delete;

	FMOD::Studio::Bank*                pBank = nullptr;
	FMOD::Studio::Bank*                pStreamsBank = nullptr;

	CryFixedStringT<MaxFilePathLength> m_streamsBankPath;
};

class CStandaloneFileBase : public IStandaloneFile
{
public:

	explicit CStandaloneFileBase(char const* const szFile, CATLStandaloneFile& atlStandaloneFile);
	virtual ~CStandaloneFileBase() override;

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

class CStandaloneFile final : public CStandaloneFileBase
{
public:

	explicit CStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile)
		: CStandaloneFileBase(szFile, atlStandaloneFile)
	{}

	// CStandaloneFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CStandaloneFileBase

	FMOD::Sound*   m_pLowLevelSound = nullptr;
	FMOD::Channel* m_pChannel = nullptr;

};

class CProgrammerSoundFile final : public CStandaloneFileBase
{
public:

	explicit CProgrammerSoundFile(char const* const szFile, FMOD_GUID const eventGuid, CATLStandaloneFile& atlStandaloneFile)
		: CStandaloneFileBase(szFile, atlStandaloneFile)
		, m_eventGuid(eventGuid)
	{}

	// CStandaloneFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CStandaloneFileBase

private:

	FMOD_GUID const              m_eventGuid;
	FMOD::Studio::EventInstance* m_pEventInstance = nullptr;

};
class CBaseObject;

using Objects = std::vector<CBaseObject*>;
using Events = std::vector<CEvent*>;
using StandaloneFiles = std::vector<CStandaloneFile*>;
using ParameterIdToIndex = std::map<uint32, int>;
using TriggerToParameterIndexes = std::map<CTrigger const* const, ParameterIdToIndex>;
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
