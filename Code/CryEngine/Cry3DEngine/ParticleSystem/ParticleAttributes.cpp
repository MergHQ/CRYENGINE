// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleCommon.h"
#include "ParticleAttributes.h"
#include "ParticleMath.h"

CRY_PFX2_DBG

namespace pfx2
{

SERIALIZATION_ENUM_IMPLEMENT(EAttributeType);
SERIALIZATION_ENUM_IMPLEMENT(EAttributeScope);

namespace
{


	const uint numTypes = IParticleAttributes::ET_Count;
	typedef CAttributeInstance::SData(*TAttribConverterFn)(CAttributeInstance::SData);
	typedef std::array<std::array<TAttribConverterFn, numTypes>, numTypes> TAttributeConverterTable;



	CAttributeInstance::SData ConvertAttrib_Bypass(CAttributeInstance::SData data)
	{
		return data;
	}

	CAttributeInstance::SData ConvertAttrib_BoolToInt(CAttributeInstance::SData data)
	{
		data.m_asInt = data.m_asBool ? 1 : 0;
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_BoolToFloat(CAttributeInstance::SData data)
	{
		data.m_asFloat = data.m_asBool ? 1.0f : 0.0f;
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_BoolToColor(CAttributeInstance::SData data)
	{
		data.m_asColor.r = data.m_asColor.g = data.m_asColor.b = data.m_asBool ? 0xff : 0x00;
		data.m_asColor.a = 0xff;
		return data;
	}

	CAttributeInstance::SData ConvertAttrib_IntToBool(CAttributeInstance::SData data)
	{
		data.m_asBool = data.m_asInt > 0 ? true : false;
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_IntToFloat(CAttributeInstance::SData data)
	{
		data.m_asFloat = float(data.m_asInt);
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_IntToColor(CAttributeInstance::SData data)
	{
		data.m_asColor.r = data.m_asColor.g = data.m_asColor.b = uint8(CLAMP(data.m_asInt, 0, 0xff));
		data.m_asColor.a = 0xff;
		return data;
	}

	CAttributeInstance::SData ConvertAttrib_FloatToBool(CAttributeInstance::SData data)
	{
		data.m_asBool = data.m_asFloat > 0.0f ? true : false;
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_FloatToInt(CAttributeInstance::SData data)
	{
		data.m_asInt = int(data.m_asFloat);
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_FloatToColor(CAttributeInstance::SData data)
	{
		data.m_asColor.r = data.m_asColor.g = data.m_asColor.b = uint8(SATURATE(data.m_asFloat)*255);
		data.m_asColor.a = 0xff;
		return data;
	}

	float Brightness(ColorB color)
	{
		return (color.r * 0.299f + color.g * 0.587f + color.b * 0.114f) / 255.0f;
	}
	CAttributeInstance::SData ConvertAttrib_ColorToBool(CAttributeInstance::SData data)
	{
		data.m_asBool = Brightness(data.m_asColor) > 0.0f ? true : false;
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_ColorToInt(CAttributeInstance::SData data)
	{
		data.m_asInt = int(Brightness(data.m_asColor) * 255.0f);
		return data;
	}
	CAttributeInstance::SData ConvertAttrib_ColorToFloat(CAttributeInstance::SData data)
	{
		data.m_asFloat = Brightness(data.m_asColor);
		return data;
	}



	const TAttributeConverterTable& GetAttribConverterTable()
	{
		static TAttributeConverterTable table;
		static bool filled = false;
		if (!filled)
		{
			for (auto& it : table)
				it.fill(ConvertAttrib_Bypass);
			table[IParticleAttributes::ET_Boolean] = { { ConvertAttrib_Bypass      , ConvertAttrib_BoolToInt  , ConvertAttrib_BoolToFloat  , ConvertAttrib_BoolToColor } };
			table[IParticleAttributes::ET_Integer] = { { ConvertAttrib_IntToBool   , ConvertAttrib_Bypass     , ConvertAttrib_IntToFloat   , ConvertAttrib_IntToColor } };
			table[IParticleAttributes::ET_Float]   = { { ConvertAttrib_FloatToBool , ConvertAttrib_FloatToInt , ConvertAttrib_Bypass       , ConvertAttrib_FloatToColor } };
			table[IParticleAttributes::ET_Color]   = { { ConvertAttrib_ColorToBool , ConvertAttrib_ColorToInt , ConvertAttrib_ColorToFloat , ConvertAttrib_Bypass } };
			filled = true;
		}

		return table;
	}


}


SAttribute::SAttribute()
	: m_type(EAttributeType::Float)
	, m_scope(EAttributeScope::PerEmitter)
	, m_asColor(~0u)
	, m_asInt(0)
	, m_minInt(0)
	, m_maxInt(10)
	, m_asFloat(1.0f)
	, m_minFloat(0.0f)
	, m_maxFloat(1.0f)
	, m_asBoolean(true)
	, m_useMin(false)
	, m_useMax(false)
{
}

void SAttribute::Serialize(Serialization::IArchive& ar)
{
	ar(m_type, "Type", "^>85>");
	ar(m_name, "Name", "^");
	ar(m_scope, "Scope", "Scope");
	switch (m_type)
	{
	case EAttributeType::Boolean:
		ar(m_asBoolean, "Value", "^");
		break;
	case EAttributeType::Integer:
		ar(m_asInt, "Value", "^");
		ar(m_useMin, "UseMin", "Use Min");
		ar(m_useMax, "UseMax", "Use Max");
		if (m_useMin)
			ar(m_minInt, "MinValue", "Minimum Value");
		if (m_useMax)
			ar(m_maxInt, "MaxValue", "Maximum Value");
		break;
	case EAttributeType::Float:
		ar(m_asFloat, "Value", "^");
		ar(m_useMin, "UseMin", "Use Min");
		ar(m_useMax, "UseMax", "Use Max");
		if (m_useMin)
			ar(m_minFloat, "MinValue", "Minimum Value");
		if (m_useMax)
			ar(m_maxFloat, "MaxValue", "Maximum Value");
		break;
	case EAttributeType::Color:
		ar(m_asColor, "Value", "^");
		break;
	}
}

//////////////////////////////////////////////////////////////////////////

void CAttributeTable::Serialize(Serialization::IArchive& ar)
{
	ar(m_attributes, "Attributes", "Attributes");
}

//////////////////////////////////////////////////////////////////////////

CAttributeInstance::CAttributeInstance()
{
}

void CAttributeInstance::Reset(IParticleAttributes* pCopySource)
{
	if (!pCopySource)
	{
		m_attributeIndices.clear();
		m_data.clear();
		m_attributeIndices.shrink_to_fit();
		m_data.shrink_to_fit();
		m_pAttributeTable.reset();
	}
	else
	{
		CAttributeInstance* pCCopySource = static_cast<CAttributeInstance*>(pCopySource);
		m_attributeIndices = pCCopySource->m_attributeIndices;
		m_data = pCCopySource->m_data;
		m_attributeIndices = pCCopySource->m_attributeIndices;
		m_pAttributeTable = pCCopySource->m_pAttributeTable;
	}
}

void CAttributeInstance::Reset(TAttributeTablePtr pTable, EAttributeScope scope)
{
	CRY_PFX2_ASSERT(pTable != nullptr);

	m_pAttributeTable = pTable;

	m_attributeIndices.clear();
	m_data.clear();

	for (size_t i = 0; i < pTable->GetNumAttributes(); ++i)
	{
		const SAttribute& attribute = pTable->GetAttribute(i);
	//	if (attribute.m_scope != scope)
	//		continue;

		SData data;
		switch (attribute.m_type)
		{
		case EAttributeType::Boolean:
			data.m_asBool = attribute.m_asBoolean;
			break;
		case EAttributeType::Integer:
			data.m_asInt = attribute.m_asInt;
			break;
		case EAttributeType::Float:
			data.m_asFloat = attribute.m_asFloat;
			break;
		case EAttributeType::Color:
			data.m_asColor = attribute.m_asColor;
			break;
		}

		m_attributeIndices.push_back(i);
		m_data.push_back(data);
	}
}

void CAttributeInstance::Serialize(Serialization::IArchive& ar)
{
	for (uint attributeId = 0; attributeId < m_attributeIndices.size(); ++attributeId)
	{
		const SAttribute& attribute = GetAttribute(attributeId);
		const auto type = IParticleAttributes::EType(attribute.m_type);
		SData data = GetAttribute(TAttributeId(attributeId), SData(), type);
		switch (attribute.m_type)
		{
		case EAttributeType::Boolean:
			ar(data.m_asBool, attribute.m_name.c_str(), attribute.m_name.c_str());
			break;
		case EAttributeType::Integer:
			ar(data.m_asInt, attribute.m_name.c_str(), attribute.m_name.c_str());
			break;
		case EAttributeType::Float:
			ar(data.m_asFloat, attribute.m_name.c_str(), attribute.m_name.c_str());
			break;
		case EAttributeType::Color:
			ar(data.m_asColor, attribute.m_name.c_str(), attribute.m_name.c_str());
			break;
		}
		if (ar.isInput())
			SetAttribute(TAttributeId(attributeId), data, type);
	}
}

void CAttributeInstance::TransferInto(IParticleAttributes* pReceiver) const
{
	CAttributeInstance* pCReceiver = static_cast<CAttributeInstance*>(pReceiver);
	auto pReceiverTable = pCReceiver->m_pAttributeTable.lock();
	auto pSourceTable = m_pAttributeTable.lock();
	CRY_PFX2_ASSERT(pSourceTable); // this instance over-lived the table

	if (pReceiverTable != pSourceTable)
	{
		pCReceiver->m_pAttributeTable = m_pAttributeTable;
		pCReceiver->m_attributeIndices = m_attributeIndices;
		pCReceiver->m_data = m_data;
	}
	else if (pCReceiver->m_attributeIndices.size() < m_attributeIndices.size())
	{
		pCReceiver->m_attributeIndices.insert(
			pCReceiver->m_attributeIndices.end(),
			m_attributeIndices.begin() + pCReceiver->m_attributeIndices.size(),
			m_attributeIndices.end());
		pCReceiver->m_data.insert(
			pCReceiver->m_data.end(),
			m_data.begin() + pCReceiver->m_data.size(),
			m_data.end());
	}
	else
	{
		pCReceiver->m_attributeIndices.resize(m_attributeIndices.size());
		pCReceiver->m_data.resize(m_data.size());
	}
}

auto CAttributeInstance::FindAttributeIdByName(cstr name) const->TAttributeId
{
	auto pAttributeTable = m_pAttributeTable.lock();
	CRY_PFX2_ASSERT(pAttributeTable); // this instance over-lived the table

	auto it = std::find_if(
	  m_attributeIndices.begin(), m_attributeIndices.end(),
	  [pAttributeTable, name](const uint entryIndex)
		{
			const SAttribute& attribute = pAttributeTable->GetAttribute(entryIndex);
			return strcmp(name, attribute.m_name.c_str()) == 0;
	  });
	if (it == m_attributeIndices.end())
		return gInvalidId;
	return TAttributeId(it - m_attributeIndices.begin());
}

uint CAttributeInstance::GetNumAttributes() const
{
	return uint(m_attributeIndices.size());
}

cstr CAttributeInstance::GetAttributeName(uint idx) const
{
	if (idx >= m_attributeIndices.size())
		return nullptr;
	return GetAttribute(m_attributeIndices[idx]).m_name.c_str();
}

auto CAttributeInstance::GetAttributeType(uint idx) const->EType
{
	if (idx >= m_attributeIndices.size())
		return ET_Float;
	return EType(GetAttribute(m_attributeIndices[idx]).m_type);
}

const SAttribute& CAttributeInstance::GetAttribute(TAttributeId attributeId) const
{
	auto pAttributeTable = m_pAttributeTable.lock();
	CRY_PFX2_ASSERT(pAttributeTable); // this instance over-lived the table
	CRY_PFX2_ASSERT(attributeId < m_attributeIndices.size());
	const uint entryIndex = m_attributeIndices[attributeId];
	const SAttribute& attribute = pAttributeTable->GetAttribute(entryIndex);
	return attribute;
}

void CAttributeInstance::SetAsBoolean(TAttributeId id, bool value)
{
	SetAttribute(id, SData(value), IParticleAttributes::ET_Boolean);
}

bool CAttributeInstance::GetAsBoolean(TAttributeId id, bool defaultValue) const
{
	return GetAttribute(id, SData(defaultValue), IParticleAttributes::ET_Boolean).m_asBool;
}

int CAttributeInstance::SetAsInteger(TAttributeId id, int value)
{
	return SetAttribute(id, SData(value), IParticleAttributes::ET_Integer).m_asInt;
}

int CAttributeInstance::GetAsInteger(TAttributeId id, int defaultValue) const
{
	return GetAttribute(id, SData(defaultValue), IParticleAttributes::ET_Integer).m_asInt;
}

float CAttributeInstance::SetAsFloat(TAttributeId id, float value)
{
	return SetAttribute(id, SData(value), IParticleAttributes::ET_Float).m_asFloat;
}

float CAttributeInstance::GetAsFloat(TAttributeId id, float defaultValue) const
{
	return GetAttribute(id, SData(defaultValue), IParticleAttributes::ET_Float).m_asFloat;
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorB value)
{
	SetAttribute(id, SData(value), IParticleAttributes::ET_Color);
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorF value)
{
	SetAsColor(id, ColorB(value));
}

ColorB CAttributeInstance::GetAsColorB(TAttributeId id, ColorB defaultValue) const
{
	return GetAttribute(id, SData(defaultValue), IParticleAttributes::ET_Color).m_asColor;
}

ColorF CAttributeInstance::GetAsColorF(TAttributeId id, ColorF defaultValue) const
{
	const ColorB color = GetAsColorB(id, ColorB(defaultValue));
	return ColorF(color.pack_abgr8888());
}

CAttributeInstance::SData CAttributeInstance::SetAttribute(TAttributeId id, const CAttributeInstance::SData& input, IParticleAttributes::EType type)
{
	const SAttribute& attribute = GetAttribute(id);
	const TAttributeConverterTable& table = GetAttribConverterTable();
	TAttribConverterFn converter = table[uint(type)][uint(attribute.m_type)];
	SData output = converter(input);

	switch (attribute.m_type)
	{
	case EAttributeType::Integer:
		if (attribute.m_useMin)
			output.m_asInt = max(attribute.m_minInt, output.m_asInt);
		if (attribute.m_useMax)
			output.m_asInt = min(attribute.m_maxInt, output.m_asInt);
		break;
	case EAttributeType::Float:
		if (attribute.m_useMin)
			output.m_asFloat = max(attribute.m_minFloat, output.m_asFloat);
		if (attribute.m_useMax)
			output.m_asFloat = min(attribute.m_maxFloat, output.m_asFloat);
		break;
	}

	m_data[id] = output;

	return output;
}

CAttributeInstance::SData CAttributeInstance::GetAttribute(TAttributeId id, const SData& defaultValue, IParticleAttributes::EType type) const
{
	if (id == gInvalidId)
		return defaultValue;
	const SAttribute& attribute = GetAttribute(id);
	const TAttributeConverterTable& table = GetAttribConverterTable();
	TAttribConverterFn converter = table[uint(attribute.m_type)][uint(type)];
	return converter(m_data[id]);
}

}
