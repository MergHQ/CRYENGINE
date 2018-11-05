// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RequestData.h"
#include "Common/PoolObject.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CListener;

enum class EListenerRequestType : EnumFlagsType
{
	None,
	SetTransformation,
	SetName,
};

//////////////////////////////////////////////////////////////////////////
struct SListenerRequestDataBase : public SRequestData
{
	explicit SListenerRequestDataBase(EListenerRequestType const listenerRequestType_)
		: SRequestData(ERequestType::ListenerRequest)
		, listenerRequestType(listenerRequestType_)
	{}

	virtual ~SListenerRequestDataBase() override = default;

	EListenerRequestType const listenerRequestType;
};

//////////////////////////////////////////////////////////////////////////
template<EListenerRequestType T>
struct SListenerRequestData final : public SListenerRequestDataBase
{
	SListenerRequestData()
		: SListenerRequestDataBase(T)
	{}

	explicit SListenerRequestData(SListenerRequestData<T> const* const pALRData)
		: SListenerRequestDataBase(T)
	{}

	virtual ~SListenerRequestData() override = default;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SListenerRequestData<EListenerRequestType::SetTransformation> final : public SListenerRequestDataBase, public CPoolObject<SListenerRequestData<EListenerRequestType::SetTransformation>, stl::PSyncMultiThread>
{
	explicit SListenerRequestData(
		CTransformation const& transformation_,
		CListener* const pListener_)
		: SListenerRequestDataBase(EListenerRequestType::SetTransformation)
		, transformation(transformation_)
		, pListener(pListener_)
	{}

	explicit SListenerRequestData(SListenerRequestData<EListenerRequestType::SetTransformation> const* const pALRData)
		: SListenerRequestDataBase(EListenerRequestType::SetTransformation)
		, transformation(pALRData->transformation)
		, pListener(pALRData->pListener)
	{}

	virtual ~SListenerRequestData() override = default;

	CTransformation const transformation;
	CListener* const      pListener;
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SListenerRequestData<EListenerRequestType::SetName> final : public SListenerRequestDataBase
{
	explicit SListenerRequestData(char const* const szName, CListener* const pListener_)
		: SListenerRequestDataBase(EListenerRequestType::SetName)
		, pListener(pListener_)
		, name(szName)
	{}

	explicit SListenerRequestData(SListenerRequestData<EListenerRequestType::SetName> const* const pALRData)
		: SListenerRequestDataBase(EListenerRequestType::SetName)
		, pListener(pALRData->pListener)
		, name(pALRData->name)
	{}

	virtual ~SListenerRequestData() override = default;

	CListener* const                           pListener;
	CryFixedStringT<MaxObjectNameLength> const name;
};
} // namespace CryAudio
