// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#ifndef PARTICLEATTRIBUTES_H
#define PARTICLEATTRIBUTES_H

#pragma once

#include "Features/ParamTraits.h"
#include <CrySerialization/Color.h>
#include <CrySerialization/Enum.h>
#include <CryCore/CryVariant.h>
#include <CryString/CryName.h>

namespace pfx2
{

SERIALIZATION_ENUM_DECLARE(EAttributeType, ,
	Boolean = IParticleAttributes::ET_Boolean,
	Integer = IParticleAttributes::ET_Integer,
	Float   = IParticleAttributes::ET_Float,
	Color   = IParticleAttributes::ET_Color
)

// A specialized color type, with built-in conversion to/from other attribute types
struct ColorAttr : ColorB
{
	using ColorB::ColorB;

	ColorAttr()            : ColorB(0xFFFFFFFF) {}

	ColorAttr(bool in)     { r = g = b = in ? 0xff : 0x00; a = 0xff; }
	ColorAttr(int in)      { r = g = b = crymath::clamp(in, 0, 0xff); a = 0xff; }
	ColorAttr(float in)    { r = g = b = float_to_ufrac8(in); a = 0xff; }

	operator bool() const  { return r + g + b > 0; }
	operator int() const   { return int_round(Luminance()); }
	operator float() const { return Luminance() / 255.0f; }
};

// Value of an attribute, implemented as a variant
using TAttributeValue = CryVariant<bool, int, float, ColorAttr>;

struct SAttributeValue: TAttributeValue
{
	using TAttributeValue::TAttributeValue;
	SAttributeValue()                            { emplace<float>(1.0f); }

	EAttributeType GetType() const               { return EAttributeType(index()); }

	template<typename T> const T& get() const    { return stl::get<T>(*this); }
	template<typename T> T& get()                { return stl::get<T>(*this); }
	template<typename T> const T* get_if() const { return stl::get_if<T>(this); }
	template<typename T> T get_as() const;

	template<typename T> void SerializeValue(IArchive& ar, cstr name, cstr label);

	void SerializeValue(IArchive& ar, EAttributeType type);
	void Serialize(IArchive& ar);
};

// Description of an attribute, including default value
struct SAttributeDesc
{
	void Serialize(IArchive& ar);

	CCryName                     m_name;
	SAttributeValue              m_defaultValue;

	TValue<int, TDefaultMin<>>   m_minInt;
	TValue<int, TDefaultMax<>>   m_maxInt;
	TValue<float, TDefaultMin<>> m_minFloat;
	TValue<float, TDefaultMax<>> m_maxFloat;

	EAttributeType GetType() const { return m_defaultValue.GetType(); }
};

// Instance of an attribute value
struct SAttributeEdit
{
	SAttributeEdit()
		{}
	SAttributeEdit(const CCryName& name, const SAttributeValue& value)
		: m_name(name), m_value(value){}

	CCryName        m_name;
	SAttributeValue m_value;

	void Serialize(IArchive& ar);
};

// List of attribute descriptions
class CAttributeTable
{
public:
	CAttributeTable()
		: m_enumDesc(yasli::TypeID::get<CAttributeTable>()) {}
                    
	uint                  GetNumAttributes() const       { return m_attributes.size(); }
	const SAttributeDesc& GetAttribute(uint index) const { return m_attributes[index]; }
	int                   FindAttributeIdByName(const CCryName& name) const;

	void                  Serialize(IArchive& ar);
	bool                  SerializeName(IArchive& ar, CCryName& value, cstr name, cstr label) const;

	void                  AddAttribute(const SAttributeDesc& desc) { m_attributes.push_back(desc);  }

private:
	std::vector<SAttributeDesc> m_attributes;
	yasli::EnumDescription      m_enumDesc; // TODO: Streamline, pass name list to archive more directly
};

typedef std::shared_ptr<const CAttributeTable> TAttributeTablePtr;

// List of attribute value instances
class CAttributeInstance : public IParticleAttributes
{
public:
	// IParticleAttributes
	virtual void         Reset(const IParticleAttributes* pCopySource = nullptr);
	virtual void         Serialize(IArchive& ar);
	virtual void         TransferInto(IParticleAttributes* pReceiver) const;
	virtual TAttributeId FindAttributeIdByName(cstr name) const { return FindAttributeIdByName(CCryName(name)); }
	virtual uint         GetNumAttributes() const;
	virtual cstr         GetAttributeName(TAttributeId idx) const;
	virtual EType        GetAttributeType(TAttributeId idx) const;
	virtual bool         GetAsBoolean(TAttributeId id, bool defaultValue) const;
	virtual int          GetAsInteger(TAttributeId id, int defaultValue) const;
	virtual float        GetAsFloat(TAttributeId id, float defaultValue) const;
	virtual ColorB       GetAsColorB(TAttributeId id, ColorB defaultValue) const;
	virtual ColorF       GetAsColorF(TAttributeId id, ColorF defaultValue) const;
	virtual void         SetAsBoolean(TAttributeId id, bool value);
	virtual int          SetAsInteger(TAttributeId id, int value);
	virtual float        SetAsFloat(TAttributeId id, float value);
	virtual void         SetAsColor(TAttributeId id, ColorB value);
	virtual void         SetAsColor(TAttributeId id, ColorF value);
	// ~IParticleAttributes

	void                   Reset(TAttributeTablePtr pTable);
	SAttributeValue        GetValue(TAttributeId id) const;
	void                   SetValue(TAttributeId id, const SAttributeValue& value);
	void                   ResetValue(TAttributeId id);
	SAttributeEdit*        FindEditById(TAttributeId id);
	const SAttributeEdit*  FindEditById(TAttributeId id) const { return non_const(this)->FindEditById(id); }
	const SAttributeDesc&  GetDesc(TAttributeId attributeId) const;

	void                   AddAttribute(const SAttributeEdit& edit) { m_attributesEdit.push_back(edit); }
	TAttributeId           FindAttributeIdByName(const CCryName& name) const;

	SAttributeValue        GetValue(TAttributeId id, SAttributeValue defaultValue) const;

	template<typename T> T GetValueAs(TAttributeId id, T defaultValue) const
	{
		return GetValue(id, defaultValue).template get<T>();
	}

	template<typename T> T SetValueAs(TAttributeId id, T input);

private:
	std::vector<SAttributeEdit>          m_attributesEdit;
	std::weak_ptr<const CAttributeTable> m_pAttributeTable;
};

class CAttributeReference
{
public:
	const CCryName& Name() const { return m_name; }

	bool Serialize(Serialization::IArchive& ar, cstr name, cstr label);

	template<typename T>
	T GetValueAs(const CAttributeInstance& attributes, T defaultValue) const
	{
		auto id = attributes.FindAttributeIdByName(m_name);
		return attributes.GetValueAs(id, defaultValue);
	}

private:
	CCryName m_name;
};

inline bool Serialize(Serialization::IArchive& ar, CAttributeReference& attr, cstr name, cstr label)
{
	return attr.Serialize(ar, name, label);
}


}

#endif
