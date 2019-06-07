// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>
#include <PoolObject.h>
#include <portaudio.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CEvent final : public ITriggerConnection, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	enum class EActionType : EnumFlagsType
	{
		None,
		Start,
		Stop,
	};

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(
		uint32 const pathId_,
		int const numLoops_,
		double const sampleRate_,
		EActionType const actionType_,
		char const* const szFilePath,
		PaStreamParameters const& streamParameters_,
		char const* const szFolder,
		char const* const szName,
		bool const isLocalized)
		: pathId(pathId_)
		, numLoops(numLoops_)
		, sampleRate(sampleRate_)
		, actionType(actionType_)
		, filePath(szFilePath)
		, streamParameters(streamParameters_)
		, m_folder(szFolder)
		, m_name(szName)
		, m_isLocalized(isLocalized)
		, m_numInstances(0)
		, m_toBeDestructed(false)
	{}

	virtual ~CEvent() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ETriggerResult Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	// ~CryAudio::Impl::ITriggerConnection

	void IncrementNumInstances() { ++m_numInstances; }
	void DecrementNumInstances();

	bool CanBeDestructed() const { return m_toBeDestructed && (m_numInstances == 0); }
	void SetToBeDestructed()     { m_toBeDestructed = true; }

	uint32 const                                pathId;
	int const                                   numLoops;
	double                                      sampleRate;
	EActionType const                           actionType;
	CryFixedStringT<MaxFilePathLength>          filePath;
	PaStreamParameters                          streamParameters;
	CryFixedStringT<MaxFilePathLength> const    m_folder;
	CryFixedStringT<MaxControlNameLength> const m_name;
	bool const                                  m_isLocalized;

private:

	uint16 m_numInstances;
	bool   m_toBeDestructed;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
