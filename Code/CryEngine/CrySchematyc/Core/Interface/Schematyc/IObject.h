// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move IObjectDump to separate header?

#pragma once

#include <CryMath/Cry_Geo.h>
#include <CrySerialization/Forward.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Reflection/TypeDesc.h"
#include "Schematyc/Runtime/RuntimeParams.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/EnumFlags.h"

// Forward declare structures.
struct SRendParams;
struct SRenderingPassInfo;

namespace Schematyc
{

// Forward declare interfaces.
struct IObjectProperties;
struct IRuntimeClass;
// Forward declare structures.

struct STimerDuration;
// Forward declare classes.
class CAction;
class CComponent;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IObjectProperties)

struct SObjectParams
{
	inline SObjectParams(const SGUID& _classGUID)
		: classGUID(_classGUID)
		, simulationMode(ESimulationMode::Idle)
	{}

	SGUID                     classGUID;
	void*                     pCustomData;
	IObjectPropertiesConstPtr pProperties;
	ESimulationMode           simulationMode;
};

enum class EObjectResetPolicy
{
	OnChange, // Only reset simulation if mode changes.
	Always    // Always reset simulation.
};

struct SObjectSignal
{
	inline SObjectSignal(const SGUID& _typeGUID, const SGUID& _senderGUID = SGUID())
		: typeGUID(_typeGUID)
		, senderGUID(_senderGUID)
	{}

	template <typename SIGNAL> inline SObjectSignal(const SIGNAL& _signal, const SGUID& _senderGUID = SGUID())
		: typeGUID(GetTypeDesc<SIGNAL>().GetGUID())
		, senderGUID(_senderGUID)
	{
		RuntimeParams::FromInputClass(params, _signal);
	}

	inline SObjectSignal(const SObjectSignal& rhs)
		: typeGUID(rhs.typeGUID)
		, senderGUID(rhs.senderGUID)
		, params(rhs.params)
	{}

	SGUID              typeGUID;
	SGUID              senderGUID;
	StackRuntimeParams params;
};

typedef CDelegate<EVisitStatus(const CComponent&)> ObjectComponentConstVisitor;

enum class EObjectDumpFlags : uint32
{
	None      = 0x00000000,
	All       = 0xffffffff,

	States    = BIT(0),
	Variables = BIT(1),
	Timers    = BIT(2)
};

typedef CEnumFlags<EObjectDumpFlags> ObjectDumpFlags;

struct IObjectDump
{
	struct SState
	{
		inline SState()
			: szName(nullptr)
		{}

		SGUID       guid;
		const char* szName;
	};

	struct SStateMachine
	{
		inline SStateMachine(const SGUID& _guid, const char* _szName)
			: guid(_guid)
			, szName(_szName)
		{}

		SGUID       guid;
		const char* szName;
		SState      state;
	};

	struct SVariable
	{
		inline SVariable(const SGUID& _guid, const char* _szName, const CAnyConstRef& _value)
			: guid(_guid)
			, szName(_szName)
			, value(_value)
		{}

		SGUID        guid;
		const char*  szName;
		CAnyConstRef value;
	};

	struct STimer
	{
		inline STimer(const SGUID& _guid, const char* _szName, const STimerDuration& _timeRemaining)
			: guid(_guid)
			, szName(_szName)
			, timeRemaining(_timeRemaining)
		{}

		SGUID                 guid;
		const char*           szName;
		const STimerDuration& timeRemaining;
	};

	virtual ~IObjectDump() {}

	virtual void operator()(const SStateMachine& stateMachine) {}
	virtual void operator()(const SVariable& variable)         {}
	virtual void operator()(const STimer& timer)               {}
};

struct IObject
{
	virtual ~IObject() {}

	virtual ObjectId                      GetId() const = 0;
	virtual const IRuntimeClass&          GetClass() const = 0;
	virtual void*                         GetCustomData() const = 0;
	virtual ESimulationMode               GetSimulationMode() const = 0;

	virtual bool                          Reset(ESimulationMode simulationMode, EObjectResetPolicy resetPolicy) = 0;
	virtual void                          ProcessSignal(const SObjectSignal& signal) = 0;
	virtual void                          StopAction(CAction& action) = 0; // #SchematycTODO : We need a better way for actions to signal that they're done! Perhaps it would be best to pass a callback?

	virtual EVisitResult                  VisitComponents(const ObjectComponentConstVisitor& visitor) const = 0;
	virtual void                          Dump(IObjectDump& dump, const ObjectDumpFlags& flags = EObjectDumpFlags::All) const = 0;

	template<typename SIGNAL> inline void ProcessSignal(const SIGNAL& signal, const SGUID& senderGUID = SGUID())
	{
		ProcessSignal(SObjectSignal(signal, senderGUID));
	}
};

struct IObjectPreviewer
{
	virtual ~IObjectPreviewer() {}

	virtual void     SerializeProperties(Serialization::IArchive& archive) = 0;
	virtual ObjectId CreateObject(const SGUID& classGUID) const = 0;
	virtual ObjectId ResetObject(ObjectId objectId) const = 0;
	virtual void     DestroyObject(ObjectId objectId) const = 0;
	virtual Sphere   GetObjectBounds(ObjectId objectId) const = 0;
	virtual void     RenderObject(const IObject& object, const SRendParams& params, const SRenderingPassInfo& passInfo) const = 0; // #SchematycTODO : Pass SRenderContext instead of SRendParams and SRenderingPassInfo?
};

} // Schematyc
