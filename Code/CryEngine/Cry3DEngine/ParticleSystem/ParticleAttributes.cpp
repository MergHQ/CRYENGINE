// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#include "StdAfx.h"
#include "ParticleCommon.h"
#include "ParticleAttributes.h"
#include "ParticleEffect.h"
#include <CrySerialization/CryName.h>
#include <CrySerialization/Decorators/ActionButton.h>

namespace pfx2
{


#pragma warning(disable: 4800) // 'int': forcing value to bool 'true' or 'false'

namespace
{
	const uint numTypes = IParticleAttributes::ET_Count;
	typedef SAttributeValue(*TAttribConverterFn)(SAttributeValue);

	template<typename TIn, typename TOut>
	SAttributeValue ConvertAttrib(SAttributeValue data)
	{
		return TOut(data.get<TIn>());
	}

	TAttribConverterFn s_AttributeConverterTable[numTypes][numTypes] =
	{
		{ ConvertAttrib<bool, bool>      , ConvertAttrib<bool, int>      , ConvertAttrib<bool, float>      , ConvertAttrib<bool, ColorAttr>   },
		{ ConvertAttrib<int, bool>       , ConvertAttrib<int, int>       , ConvertAttrib<int, float>       , ConvertAttrib<int, ColorAttr>    },
		{ ConvertAttrib<float, bool>     , ConvertAttrib<float, int>     , ConvertAttrib<float, float>     , ConvertAttrib<float, ColorAttr>  },
		{ ConvertAttrib<ColorAttr, bool> , ConvertAttrib<ColorAttr, int> , ConvertAttrib<ColorAttr, float> , ConvertAttrib<ColorAttr, ColorAttr> }
	};
	
	struct AttributeEditSerializer
	{
		CAttributeInstance* m_pAttributes;
		uint                m_index;

		void Serialize(IArchive& ar)
		{
			const SAttributeDesc& desc = m_pAttributes->GetDesc(m_index);
			SAttributeValue value = m_pAttributes->GetValue(m_index);

			value.SerializeValue(ar, desc.GetType());
			if (ar.isInput())
				m_pAttributes->SetValue(m_index, value);

			bool edited = value != desc.m_defaultValue;
			ar(Serialization::ActionButton(*this), "Reset", edited ? "^Reset" : "!^Reset");

		}
		// Reset function
		void operator()() const
		{
			m_pAttributes->ResetValue(m_index);
		}
	};
}

template<typename T>
void SAttributeValue::SerializeValue(IArchive& ar, cstr name, cstr label)
{
	const T* pVal = get_if<T>();
	T val = pVal ? *pVal : T();

	ar(val, name, label);

	if (ar.isInput())
		*this = val;
}

void SAttributeValue::SerializeValue(IArchive& ar, EAttributeType type)
{
	switch (type)
	{
	case EAttributeType::Boolean:
		return SerializeValue<bool>(ar, "Value", "^");
	case EAttributeType::Integer:
		return SerializeValue<int>(ar, "Value", "^");
	case EAttributeType::Float:
		return SerializeValue<float>(ar, "Value", "^");
	case EAttributeType::Color:
		return SerializeValue<ColorAttr>(ar, "Value", "^");
	}
}

void SAttributeValue::Serialize(IArchive& ar)
{
	EAttributeType type = GetType();
	ar(type, "Type", "^>85>");
	SerializeValue(ar, type);
}

void SAttributeDesc::Serialize(IArchive& ar)
{
	ar(m_name, "Name", "Name");
	m_defaultValue.Serialize(ar);

	if (m_defaultValue.get_if<int>())
	{
		ar(m_minInt, "MinValue", "Minimum Value");
		ar(m_maxInt, "MaxValue", "Maximum Value");
	}
	else if (m_defaultValue.get_if<float>())
	{
		ar(m_minFloat, "MinValue", "Minimum Value");
		ar(m_maxFloat, "MaxValue", "Maximum Value");
	}
}

int CAttributeTable::FindAttributeIdByName(const CCryName& name) const
{
	return stl::find_index_if(m_attributes,
		[&name](const SAttributeDesc& attr)
		{
			return attr.m_name == name;
		});
}

//////////////////////////////////////////////////////////////////////////

void CAttributeTable::Serialize(IArchive& ar)
{
	ar(m_attributes, "Attributes", "Attributes");
	if (ar.isInput())
	{
		m_enumDesc = yasli::EnumDescription(yasli::TypeID::get<CAttributeTable>());
		int i = -1;
		m_enumDesc.add(i++, "", "");
		for (const auto& attr : m_attributes)
			m_enumDesc.add(i++, attr.m_name.c_str(), attr.m_name.c_str());
	}
}

bool CAttributeTable::SerializeName(IArchive& ar, CCryName& value, cstr name, cstr label) const
{
	if (ar.isEdit())
	{
		if (m_enumDesc.count())
		{
			int attributeId = FindAttributeIdByName(value);
			if (!m_enumDesc.serialize(ar, attributeId, name, label))
				return false;
			value = attributeId >= 0 ? m_attributes[attributeId].m_name : CCryName();
			return true;
		}
	}
	return ar(value, name, label);
}

//////////////////////////////////////////////////////////////////////////

void SAttributeEdit::Serialize(IArchive& ar)
{
	SERIALIZE_VAR(ar, m_name);
	SERIALIZE_VAR(ar, m_value);
}

//////////////////////////////////////////////////////////////////////////

void CAttributeInstance::Reset(const IParticleAttributes* pCopySource)
{
	if (!pCopySource)
	{
		m_attributesEdit.clear();
		m_pAttributeTable.reset();
	}
	else
	{
		const CAttributeInstance* pCCopySource = static_cast<const CAttributeInstance*>(pCopySource);
		m_attributesEdit = pCCopySource->m_attributesEdit;
		m_pAttributeTable = pCCopySource->m_pAttributeTable;
	}
}

void CAttributeInstance::Reset(TAttributeTablePtr pTable)
{
	CRY_PFX2_ASSERT(pTable != nullptr);

	if (m_pAttributeTable.lock() != pTable)
	{
		m_pAttributeTable = pTable;
	}
}

void CAttributeInstance::Serialize(IArchive& ar)
{
	if (!ar.isEdit())
	{
		ar(m_attributesEdit, "Attributes", "Attributes");
	}
	else
	{
		uint numAttributes = GetNumAttributes();
		for (uint attributeId = 0; attributeId < numAttributes; ++attributeId)
		{
			const SAttributeDesc& attribute = GetDesc(attributeId);
			const char* name = attribute.m_name.c_str();
			ar(AttributeEditSerializer{ this, attributeId }, name, name);
		}
	}
}

void CAttributeInstance::TransferInto(IParticleAttributes* pReceiver) const
{
	CAttributeInstance* pCReceiver = static_cast<CAttributeInstance*>(pReceiver);
	auto pReceiverTable = pCReceiver->m_pAttributeTable.lock();
	auto pSourceTable = m_pAttributeTable.lock();
	CRY_PFX2_ASSERT(pSourceTable); // this instance over-lived the table

	pCReceiver->m_pAttributeTable = m_pAttributeTable;

	// Merge existing edits
	for (const auto& edit : m_attributesEdit)
	{
		if (pCReceiver->FindAttributeIdByName(edit.m_name) == gInvalidId)
			pCReceiver->AddAttribute(edit);
	}
}

auto CAttributeInstance::FindAttributeIdByName(const CCryName& name) const->TAttributeId
{
	if (auto pAttributeTable = m_pAttributeTable.lock())
		return pAttributeTable->FindAttributeIdByName(name);
	return gInvalidId;
}

uint CAttributeInstance::GetNumAttributes() const
{
	if (auto pAttributeTable = m_pAttributeTable.lock())
		return pAttributeTable->GetNumAttributes();
	return 0;
}

cstr CAttributeInstance::GetAttributeName(uint id) const
{
	return GetDesc(id).m_name.c_str();
}

auto CAttributeInstance::GetAttributeType(uint id) const->EType
{
	return (EType)GetDesc(id).GetType();
}

const SAttributeDesc& CAttributeInstance::GetDesc(uint id) const
{
	if (auto pAttributeTable = m_pAttributeTable.lock())
	{
		if (id < pAttributeTable->GetNumAttributes())
			return pAttributeTable->GetAttribute(id);
	}
	static SAttributeDesc emptyDesc;
	return emptyDesc;
}

void CAttributeInstance::SetAsBoolean(TAttributeId id, bool value)
{
	SetValueAs(id, value);
}

bool CAttributeInstance::GetAsBoolean(TAttributeId id, bool defaultValue) const
{
	return GetValueAs(id, defaultValue);
}

int CAttributeInstance::SetAsInteger(TAttributeId id, int value)
{
	return SetValueAs(id, value);
}

int CAttributeInstance::GetAsInteger(TAttributeId id, int defaultValue) const
{
	return GetValueAs(id, defaultValue);
}

float CAttributeInstance::SetAsFloat(TAttributeId id, float value)
{
	return SetValueAs(id, value);
}

float CAttributeInstance::GetAsFloat(TAttributeId id, float defaultValue) const
{
	return GetValueAs(id, defaultValue);
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorB value)
{
	SetValueAs(id, ColorAttr(value));
}

void CAttributeInstance::SetAsColor(TAttributeId id, ColorF value)
{
	SetValueAs(id, ColorAttr(value));
}

ColorB CAttributeInstance::GetAsColorB(TAttributeId id, ColorB defaultValue) const
{
	return GetValueAs(id, ColorAttr(defaultValue));
}

ColorF CAttributeInstance::GetAsColorF(TAttributeId id, ColorF defaultValue) const
{
	if (id >= GetNumAttributes())
		return defaultValue;
	const ColorB color = GetValueAs(id, ColorAttr(defaultValue));
	return ColorF(color.pack_abgr8888());
}

SAttributeEdit* CAttributeInstance::FindEditById(TAttributeId id)
{
	const CCryName& name = GetDesc(id).m_name;
	const int editId = stl::find_index_if(m_attributesEdit, [&name](const SAttributeEdit& edit)
	{
		return edit.m_name == name;
	});
	return editId >= 0 ? &m_attributesEdit[editId] : nullptr;
}

SAttributeValue CAttributeInstance::GetValue(TAttributeId id) const
{
	if (const auto* pEdit = FindEditById(id))
		return pEdit->m_value;
	return GetDesc(id).m_defaultValue;
}

void CAttributeInstance::SetValue(TAttributeId id, const SAttributeValue& input)
{
	if (id >= GetNumAttributes())
		return;

	const SAttributeDesc& desc = GetDesc(id);
	TAttribConverterFn converter = s_AttributeConverterTable[input.index()][(uint)desc.GetType()];
	SAttributeValue output = converter(input);

	if (output.get_if<int>())
	{
		output = max(+desc.m_minInt, output.get<int>());
		output = min(+desc.m_maxInt, output.get<int>());
	}
	else if (output.get_if<float>())
	{
		output = max(+desc.m_minFloat, output.get<float>());
		output = min(+desc.m_maxFloat, output.get<float>());
	}

	if (auto* pEdit = FindEditById(id))
		pEdit->m_value = output;
	else
		m_attributesEdit.emplace_back(GetDesc(id).m_name, output);
}

void CAttributeInstance::ResetValue(TAttributeId id)
{
	const CCryName& name = GetDesc(id).m_name;
	stl::find_and_erase_if(m_attributesEdit, [&name](const SAttributeEdit& edit)
	{
		return edit.m_name == name;
	});
}

SAttributeValue CAttributeInstance::GetValue(TAttributeId id, SAttributeValue defaultValue) const
{
	if (id >= GetNumAttributes())
		return defaultValue;

	SAttributeValue value = GetValue(id);
	TAttribConverterFn converter = s_AttributeConverterTable[value.index()][defaultValue.index()];
	return converter(value);
}

template<typename T> T CAttributeInstance::SetValueAs(TAttributeId id, T value)
{
	SAttributeValue input = value;
	SetValue(id, input);

	SAttributeValue output = GetValue(id);
	TAttribConverterFn converter = s_AttributeConverterTable[output.index()][input.index()];
	output = converter(output);

	return output.get<T>();
}

bool CAttributeReference::Serialize(Serialization::IArchive& ar, cstr name, cstr label)
{
	if (!ar.isEdit())
		return ar(m_name, name, label);
	else if (auto* pComponent = ar.context<IParticleComponent>())
	{
		if (auto* pEffect = static_cast<CParticleComponent*>(pComponent)->GetEffect())
		{
			const auto& attributes = pEffect->GetAttributeTable();
			return attributes->SerializeName(ar, m_name, name, label);
		}
	}
	return false;
}

}
