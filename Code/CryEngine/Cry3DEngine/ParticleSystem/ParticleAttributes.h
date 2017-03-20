// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef PARTICLEATTRIBUTES_H
#define PARTICLEATTRIBUTES_H

#pragma once

#include <CrySerialization/Color.h>
#include <CrySerialization/Enum.h>

namespace pfx2
{

SERIALIZATION_ENUM_DECLARE(EAttributeType, ,
                           Boolean = IParticleAttributes::ET_Boolean,
                           Integer = IParticleAttributes::ET_Integer,
                           Float = IParticleAttributes::ET_Float,
                           Color = IParticleAttributes::ET_Color
                           )

SERIALIZATION_ENUM_DECLARE(EAttributeScope, ,
                           PerEmitter,
                           PerEffect
                           )

struct SAttribute
{
	SAttribute();

	void Serialize(Serialization::IArchive& ar);

	string          m_name;
	EAttributeType  m_type;
	EAttributeScope m_scope;
	ColorB          m_asColor;
	int             m_minInt;
	int             m_maxInt;
	float           m_asFloat;
	float           m_minFloat;
	float           m_maxFloat;
	int             m_asInt;
	bool            m_asBoolean;
	bool            m_useMin;
	bool            m_useMax;
};

class CAttributeTable
{
public:
	size_t            GetNumAttributes() const       { return m_attributes.size(); }
	const SAttribute& GetAttribute(size_t idx) const { return m_attributes[idx]; }
	void              Serialize(Serialization::IArchive& ar);

private:
	std::vector<SAttribute>           m_attributes;
	IParticleAttributes::TAttributeId m_nextId = 0;
};

typedef std::shared_ptr<CAttributeTable> TAttributeTablePtr;

class CAttributeInstance : public IParticleAttributes
{
public:
	struct SData
	{
		SData() = default;
		explicit SData(ColorB value) : m_asColor(value) {}
		explicit SData(int value) : m_asInt(value) {}
		explicit SData(float value) : m_asFloat(value) {}
		explicit SData(bool value) : m_asBool(value) {}
		ColorB m_asColor;		
		int    m_asInt;
		float  m_asFloat;
		bool   m_asBool;
	};

public:
	CAttributeInstance();

	// IParticleAttributes
	virtual void         Reset(IParticleAttributes* pCopySource = nullptr);
	virtual void         Serialize(Serialization::IArchive& ar);
	virtual void         TransferInto(IParticleAttributes* pReceiver) const;
	virtual TAttributeId FindAttributeIdByName(cstr name) const;
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

	void              Reset(TAttributeTablePtr pTable, EAttributeScope scope);
	const SAttribute& GetAttribute(TAttributeId attributeId) const;

private:
	SData SetAttribute(TAttributeId id, const SData& input, IParticleAttributes::EType type);
	SData GetAttribute(TAttributeId id, const SData& defaultValue, IParticleAttributes::EType type) const;

private:
	std::vector<uint>                    m_attributeIndices;
	std::vector<SData>                   m_data;
	std::weak_ptr<const CAttributeTable> m_pAttributeTable;
};

}

#endif
