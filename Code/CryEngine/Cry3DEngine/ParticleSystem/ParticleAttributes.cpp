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
	: m_pAttributeTable(nullptr)
{
}

void CAttributeInstance::Reset()
{
	m_attributeIndices.clear();
	m_data.clear();
	m_attributeIndices.shrink_to_fit();
	m_data.shrink_to_fit();
	m_pAttributeTable = nullptr;
}

void CAttributeInstance::Reset(const CAttributeTable* pTable, EAttributeScope scope)
{
	CRY_PFX2_ASSERT(pTable != nullptr);

	m_pAttributeTable = pTable;

	m_attributeIndices.clear();
	m_data.clear();

	for (size_t i = 0; i < pTable->GetNumAttributes(); ++i)
	{
		const SAttribute& attribute = pTable->GetAttribute(i);
		if (attribute.m_scope != scope)
			continue;

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
			data.m_asColor.dcolor = attribute.m_asColor.pack_rgb888();
			break;
		}

		m_attributeIndices.push_back(i);
		m_data.push_back(data);
	}
}

void CAttributeInstance::UpdateScriptTable(const SmartScriptTable& scriptTable)
{
	for (TAttributeId attributeId = 0; attributeId < m_attributeIndices.size(); ++attributeId)
	{
		const SAttribute& attribute = GetAttribute(attributeId);
		if (attribute.m_scope != EAttributeScope::PerEmitter)
			continue;

		switch (attribute.m_type)
		{
		case EAttributeType::Boolean:
			{
				bool value = GetAsBoolean(attributeId, attribute.m_asBoolean);
				if (scriptTable->GetValue(attribute.m_name.c_str(), value))
					SetAsBoolean(attributeId, value);
				else
					scriptTable->SetValue(attribute.m_name.c_str(), value);
			}
			break;
		case EAttributeType::Float:
			{
				float value = GetAsFloat(attributeId, attribute.m_asFloat);
				if (scriptTable->GetValue(attribute.m_name.c_str(), value))
					value = SetAsFloat(attributeId, value);
				scriptTable->SetValue(attribute.m_name.c_str(), value);
			}
			break;
		case EAttributeType::Color:
			{
				Vec3 value = GetAsColorF(attributeId, ColorF(attribute.m_asColor.toVec3())).toVec3();
				if (scriptTable->GetValue(attribute.m_name.c_str(), value))
					SetAsColor(attributeId, ColorF(value));
				else
					scriptTable->SetValue(attribute.m_name.c_str(), value);
			}
			break;
		}
	}
}

auto CAttributeInstance::FindAttributeIdByName(cstr name) const->TAttributeId
{
	auto it = std::find_if(
	  m_attributeIndices.begin(), m_attributeIndices.end(),
	  [this, name](const uint entryIndex)
		{
			const SAttribute& attribute = m_pAttributeTable->GetAttribute(entryIndex);
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
	CRY_PFX2_ASSERT(attributeId < m_attributeIndices.size());
	const uint entryIndex = m_attributeIndices[attributeId];
	const SAttribute& attribute = m_pAttributeTable->GetAttribute(entryIndex);
	return attribute;
}

void CAttributeInstance::SetAsBoolean(TAttributeId id, bool value)
{
	if (id >= m_data.size())
		return;
	const SAttribute& attribute = GetAttribute(id);
	SData& data = m_data[id];
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		data.m_asBool = value;
		break;
	case EAttributeType::Integer:
		data.m_asInt = value ? 1 : 0;
		break;
	case EAttributeType::Float:
		data.m_asFloat = value ? 1.0f : 0.0f;
		break;
	case EAttributeType::Color:
		data.m_asColor.r = data.m_asColor.g = data.m_asColor.b = value ? 0xFF : 0x00;
		data.m_asColor.a = 0xff;
		break;
	}
}

bool CAttributeInstance::GetAsBoolean(TAttributeId id, bool defaultValue) const
{
	if (id >= m_data.size())
		return defaultValue;
	const SAttribute& attribute = GetAttribute(id);
	const SData& data = m_data[id];
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		return data.m_asBool;
	case EAttributeType::Integer:
		return data.m_asInt > 0 ? true : false;
	case EAttributeType::Float:
		return data.m_asFloat > 0.0f ? true : false;
	}
	return defaultValue;
}

int CAttributeInstance::SetAsInteger(TAttributeId id, int value)
{
	return value;
}

int CAttributeInstance::GetAsInteger(TAttributeId id, int defaultValue) const
{
	return defaultValue;
}

float CAttributeInstance::SetAsFloat(TAttributeId id, float value)
{
	if (id >= m_data.size())
		return value;

	const SAttribute& attribute = GetAttribute(id);
	float actualValue = value;
	if (attribute.m_useMin)
		actualValue = max(attribute.m_minFloat, actualValue);
	if (attribute.m_useMax)
		actualValue = min(attribute.m_maxFloat, actualValue);

	SData& data = m_data[id];
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		data.m_asBool = actualValue > 0.0f ? true : false;
		break;
	case EAttributeType::Integer:
		data.m_asInt = int(actualValue);
		break;
	case EAttributeType::Float:
		data.m_asFloat = value;
		break;
	case EAttributeType::Color:
		data.m_asColor.r = data.m_asColor.g = data.m_asColor.b = float_to_ufrac8(actualValue);
		data.m_asColor.a = 0xff;
		break;
	}

	return actualValue;
}

float CAttributeInstance::GetAsFloat(TAttributeId id, float defaultValue) const
{
	if (id >= m_data.size())
		return defaultValue;
	const SAttribute& attribute = GetAttribute(id);
	const SData& data = m_data[id];
	float result = defaultValue;
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		result = data.m_asBool ? 1.0f : 0.0f;
		break;
	case EAttributeType::Integer:
		result = float(data.m_asInt);
		break;
	case EAttributeType::Float:
		result = data.m_asFloat;
		break;
	case EAttributeType::Color:
		{
			const Vec3 colorv = attribute.m_asColor.toVec3();
			result = colorv.x * 0.299f + colorv.y * 0.587f + colorv.z * 0.114f;
		}
	}
	return result;
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorB value)
{
	SetAsColor(id, ColorF(value.toVec3()));
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorF value)
{
	if (id >= m_data.size())
		return;
	const SAttribute& attribute = GetAttribute(id);
	SData& data = m_data[id];
	float brightness = value.r * 0.299f + value.g * 0.587f + value.b * 0.114f;
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		data.m_asBool = (brightness > 0.0f);
		break;
	case EAttributeType::Integer:
		data.m_asInt = int(brightness * 255.0f);
		break;
	case EAttributeType::Float:
		data.m_asFloat = brightness;
		break;
	case EAttributeType::Color:
		data.m_asColor = ColorFToUCol(value);
		break;
	}
}

ColorB CAttributeInstance::GetAsColorB(TAttributeId id, ColorB defaultValue) const
{
	return ColorB(GetAsColorF(id, ColorF(defaultValue.toVec3())));
}

ColorF CAttributeInstance::GetAsColorF(TAttributeId id, ColorF defaultValue) const
{
	if (id >= m_data.size())
		return defaultValue;
	const SAttribute& attribute = GetAttribute(id);
	const SData& data = m_data[id];
	switch (attribute.m_type)
	{
	case EAttributeType::Boolean:
		return data.m_asBool ? ColorF(1.0f, 1.0f, 1.0f) : ColorF(0.0f, 0.0f, 0.0f);
	case EAttributeType::Integer:
		return ColorF(float(data.m_asInt), float(data.m_asInt), float(data.m_asInt)) / 255.0f;
	case EAttributeType::Float:
		return ColorF(data.m_asFloat, data.m_asFloat, data.m_asFloat);
	case EAttributeType::Color:
		return ToColorF(data.m_asColor);
	}
	return defaultValue;
}

}
