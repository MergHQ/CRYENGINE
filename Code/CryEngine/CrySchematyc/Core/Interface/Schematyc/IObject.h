// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move IObjectDump to separate header?

#pragma once

#include <CrySerialization/Forward.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Reflection/Reflection.h"
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
struct SGUID;
struct STimerDuration;
// Forward declare classes.
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
	OnChange,   // Only reset simulation if mode changes.
	Always      // Always reset simulation.
};

typedef CDelegate<EVisitStatus (const CComponent&)> ObjectComponentConstVisitor;

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
	virtual void                          ProcessSignal(const SGUID& signalGUID, CRuntimeParams& params) = 0;

	virtual EVisitResult                  VisitComponents(const ObjectComponentConstVisitor& visitor) const = 0;
	virtual void                          Dump(IObjectDump& dump, const ObjectDumpFlags& flags = EObjectDumpFlags::All) const = 0;

	template<typename SIGNAL> inline void ProcessSignal(const SIGNAL& signal)
	{
		StackRuntimeParams params;
		const CCommonTypeInfo& typeInfo = GetTypeInfo<SIGNAL>();
		if (typeInfo.GetClassification() == ETypeClassification::Class)
		{
			for (const SClassTypeInfoMember& member : static_cast<const CClassTypeInfo&>(typeInfo).GetMembers())
			{
				params.SetInput(member.id, CAnyConstRef(member.typeInfo, reinterpret_cast<const uint8*>(&signal) + member.offset));
			}
		}
		ProcessSignal(typeInfo.GetGUID(), params);
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
