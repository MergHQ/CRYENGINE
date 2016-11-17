// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move contents of ScriptVariableData namespace to separate file and separate namespace? ScriptTypeUtils perhaps?

#pragma once

#include <CrySerialization/Forward.h>
#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Script/ScriptDependencyEnumerator.h>
#include <Schematyc/SerializationUtils/SerializationQuickSearch.h>
#include <Schematyc/Utils/Delegate.h>
#include <Schematyc/Utils/EnumFlags.h>
#include <Schematyc/Utils/GUID.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvDataType;
struct IGUIDRemapper;
struct IScriptEnum;
struct IScriptStruct;
// Forward declare classes.
class CAnyConstPtr;
class CAnyValue;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAnyValue)

class CScriptVariableData
{
public:

	CScriptVariableData(const SElementId& typeId = SElementId(), bool bSupportsArray = false);

	bool         IsEmpty() const;
	SElementId   GetTypeId() const;
	const char*  GetTypeName() const;
	bool         SupportsArray() const;
	bool         IsArray() const;
	CAnyConstPtr GetValue() const;

	void         EnumerateDependencies(const ScriptDependencyEnumerator& enumerator, EScriptDependencyType type) const;
	void         RemapDependencies(IGUIDRemapper& guidRemapper);
	void         SerializeTypeId(Serialization::IArchive& archive);
	void         SerializeValue(Serialization::IArchive& archive); // #SchematycTODO : Rename SerializeValue()!!!
	void         Refresh();

private:

	SElementId   m_typeId;
	bool         m_bSupportsArray;
	bool         m_bIsArray;
	CAnyValuePtr m_pValue;
};

namespace ScriptVariableData
{
typedef CDelegate<bool (const IEnvDataType&)>  EnvDataTypeFilter;
typedef CDelegate<bool (const IScriptEnum&)>   ScriptEnumsFilter;
typedef CDelegate<bool (const IScriptStruct&)> ScriptStructFilter;

class CScopedSerializationConfig
{
public:

	CScopedSerializationConfig(Serialization::IArchive& archive, const char* szHeader = nullptr);

	void DeclareEnvDataTypes(const SGUID& scopeGUID, const EnvDataTypeFilter& filter = EnvDataTypeFilter());
	void DeclareScriptEnums(const SGUID& scopeGUID, const ScriptEnumsFilter& filter = ScriptEnumsFilter());
	void DeclareScriptStructs(const SGUID& scopeGUID, const ScriptStructFilter& filter = ScriptStructFilter());

private:

	SerializationUtils::CScopedQuickSearchConfig<SElementId> m_typeIdQuickSearchConfig;
};

CAnyValuePtr CreateData(const SElementId& typeId);      // #SchematycTODO : Can we make this private within the cpp file?
CAnyValuePtr CreateArrayData(const SElementId& typeId); // #SchematycTODO : Can we make this private within the cpp file?

} // ScriptVariableData
} // Schematyc
