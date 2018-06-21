// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryReflection/Debug/NatVisWriter.h>

#include <CryReflection/Any.h>
#include <CryReflection/ITypeDesc.h>

#include <CryMath/Cry_Math.h>
#include <CrySystem/XML/IXml.h>
#include <CryString/CryFixedString.h>

namespace Cry {
namespace Reflection {

inline bool NatVisWriter::IsFundamental(Type::CTypeId typeId)
{
	return (
	  (typeId == Type::IdOf<bool>()) ||
	  (typeId == Type::IdOf<char>()) ||
	  // Float needs special care to correctly show in the watch window.
	  //(typeId == Type::IdOf<float>()) ||
	  (typeId == Type::IdOf<uint64>()) ||
	  (typeId == Type::IdOf<uint32>()) ||
	  (typeId == Type::IdOf<uint16>()) ||
	  (typeId == Type::IdOf<uint8>()) ||
	  (typeId == Type::IdOf<int64>()) ||
	  (typeId == Type::IdOf<int32>()) ||
	  (typeId == Type::IdOf<int16>()) ||
	  (typeId == Type::IdOf<int8>()));
}

inline bool NatVisWriter::WriteFile()
{
	const char* szNatVisFileName = "Reflection.natvis";
	const char* szReflectionPath = "Code\\CryEngine\\CryReflection\\";

	const char* szTypeDesc = "((CryReflection.dll!Cry::Reflection::CModule*)gEnv->pReflection)->m_coreRegistry.m_typesByIndex[m_typeIndex.m_value]";
	const char* szConditionFmt = "m_typeIndex.m_value == %u && m_isPointer == %s";

	XmlNodeRef rootNode = GetISystem()->CreateXmlNode("AutoVisualizer");
	rootNode->setAttr("xmlns", "http://schemas.microsoft.com/vstudio/debugger/natvis/2010");

	// Reflection::CAny
	{
		XmlNodeRef typeNode = rootNode->newChild("Type");
		typeNode->setAttr("Name", "Cry::Reflection::CAny");

		const TypeIndex::ValueType typeCount = CoreRegistry::GetTypeCount();
		for (TypeIndex::ValueType i = 0; i < typeCount; ++i)
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(i);
			const Type::CTypeId typeId = pTypeDesc->GetTypeId();

			const char* szValueFormat = nullptr;
			const char* szPointerFormat = nullptr;
			const char* szRawName = pTypeDesc->GetFullQualifiedName();
			if (IsFundamental(typeId))
			{
				szValueFormat = "{(%s)m_data} (%s)";
				szPointerFormat = "{(%s*)m_data} (%s*)";

				// WORKAROUND: (signed char) cast is broken in VS watch window -> use char instead
				if (typeId == Type::IdOf<int8>())
				{
					szRawName = "char";
				}
				// ~WORKAROUND
			}
			else
			{
				const bool hasMemoryAllocated = (pTypeDesc->GetSize() > CAny::GetFixedBufferSize());
				szValueFormat = hasMemoryAllocated ? "{*(%s*)m_data} (%s)" : "{*(%s*)&m_data} (%s)";
				szPointerFormat = hasMemoryAllocated ? "{(%s*)m_data} (%s*)" : "{(%s*)&m_data} (%s*)";
			}

			// Value
			XmlNodeRef displayNode = typeNode->newChild("DisplayString");
			displayNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "false"));
			displayNode->setContent(string().Format(szValueFormat, szRawName, pTypeDesc->GetLabel()));

			// Pointer
			displayNode = typeNode->newChild("DisplayString");
			displayNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "true"));
			displayNode->setContent(string().Format(szPointerFormat, szRawName, pTypeDesc->GetLabel()));
		}

		// Expansion
		XmlNodeRef expandNode = typeNode->newChild("Expand");

		// Item Value
		for (TypeIndex::ValueType i = 0; i < typeCount; ++i)
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(i);
			const Type::CTypeId typeId = pTypeDesc->GetTypeId();

			const char* szValueFormat = nullptr;
			const char* szPointerFormat = nullptr;
			const char* szRawName = pTypeDesc->GetFullQualifiedName();
			if (IsFundamental(typeId))
			{
				szValueFormat = "(%s)m_data";
				szPointerFormat = "(%s*)m_data";
				// WORKAROUND: (signed char) cast is broken in VS watch window -> use char instead
				if (typeId == Type::IdOf<int8>())
				{
					szRawName = "char";
				}
				// ~WORKAROUND
			}
			else
			{
				const bool hasMemoryAllocated = (pTypeDesc->GetSize() > CAny::GetFixedBufferSize());
				szValueFormat = hasMemoryAllocated ? "*(%s*)m_data" : "*(%s*)&m_data";
				szPointerFormat = hasMemoryAllocated ? "(%s*)m_data" : "(%s*)&m_data";
			}

			// Value
			XmlNodeRef itemNode = expandNode->newChild("Item");
			itemNode->setAttr("Name", "Value");
			itemNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "false"));
			itemNode->setContent(string().Format(szValueFormat, szRawName, pTypeDesc->GetLabel()));

			// Pointer
			itemNode = expandNode->newChild("Item");
			itemNode->setAttr("Name", "Pointer");
			itemNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "true"));
			itemNode->setContent(string().Format(szPointerFormat, szRawName, pTypeDesc->GetLabel()));
		}

		// GUID
		XmlNodeRef syntheticNode = expandNode->newChild("Synthetic");
		syntheticNode->setAttr("Name", "GUID");
		XmlNodeRef displayStringNode = syntheticNode->newChild("DisplayString");
		displayStringNode->setContent(string().Format("{%s%s}", szTypeDesc, ".m_guid"));

		// TypeDesc
		XmlNodeRef itemNode = expandNode->newChild("Item");
		itemNode->setAttr("Name", "TypeDesc");
		itemNode->setContent(szTypeDesc);
	}

	// Reflection::CAnyArray
	{
		XmlNodeRef typeNode = rootNode->newChild("Type");
		typeNode->setAttr("Name", "Cry::Reflection::CAnyArray");

		const TypeIndex::ValueType typeCount = CoreRegistry::GetTypeCount();
		for (TypeIndex::ValueType i = 0; i < typeCount; ++i)
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(i);

			// Value
			XmlNodeRef displayNode = typeNode->newChild("DisplayString");
			displayNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "false"));
			displayNode->setContent(string().Format("{{ size = {m_size}, capacity = {m_capacity} }} (%s)", pTypeDesc->GetLabel()));

			// Pointer
			displayNode = typeNode->newChild("DisplayString");
			displayNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "true"));
			displayNode->setContent(string().Format("{{ size = {m_size}, capacity = {m_capacity} }} (%s*)", pTypeDesc->GetLabel()));
		}

		// Expansion
		XmlNodeRef expandNode = typeNode->newChild("Expand");

		// GUID
		XmlNodeRef syntheticNode = expandNode->newChild("Synthetic");
		syntheticNode->setAttr("Name", "GUID");
		XmlNodeRef displayStringNode = syntheticNode->newChild("DisplayString");
		displayStringNode->setContent(string().Format("{%s%s}", szTypeDesc, ".m_guid"));

		// TypeDesc
		XmlNodeRef itemNode = expandNode->newChild("Item");
		itemNode->setAttr("Name", "TypeDesc");
		itemNode->setContent(szTypeDesc);

		// Array Items
		XmlNodeRef arrayItemsNode = expandNode->newChild("ArrayItems");
		XmlNodeRef sizeNode = arrayItemsNode->newChild("Size");
		sizeNode->setContent("m_size");

		for (TypeIndex::ValueType i = 0; i < typeCount; ++i)
		{
			const ITypeDesc* pTypeDesc = CoreRegistry::GetTypeByIndex(i);

			// Value
			XmlNodeRef valuePointerNode = arrayItemsNode->newChild("ValuePointer");
			valuePointerNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "false"));
			valuePointerNode->setContent(string().Format("(%s*)m_pData", pTypeDesc->GetFullQualifiedName()));

			// Pointer
			valuePointerNode = arrayItemsNode->newChild("ValuePointer");
			valuePointerNode->setAttr("Condition", string().Format(szConditionFmt, pTypeDesc->GetIndex().ToValueType(), "true"));
			valuePointerNode->setContent(string().Format("(%s**)m_pData", pTypeDesc->GetFullQualifiedName()));
		}
	}

	stack_string path;
	path.append(gEnv->pSystem->GetRootFolder());
	path.append(szReflectionPath);
	path.append(szNatVisFileName);

	return rootNode->saveToFile(path.c_str());
}

} // ~Reflection namespace
} // ~Cry namespace
