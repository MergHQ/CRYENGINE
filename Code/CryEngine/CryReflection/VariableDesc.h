// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DescExtension.h"

#include <CryReflection/IVariableDesc.h>
#include <CryString/CryString.h>

namespace Cry {
namespace Reflection {

class CTypeDesc;

class CVariableDesc : public CExtensibleDesc<IVariableDesc>
{
public:
	CVariableDesc(void* pAddress, const char* szLabel, CTypeId typeId, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcInfo);
	CVariableDesc(ptrdiff_t offset, const char* szLabel, const CTypeDesc& parentTypeDesc, CTypeId typeId, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcInfo);

	// IVariableDesc
	virtual CGuid                  GetGuid() const final                           { return m_guid; }
	virtual CTypeId                GetTypeId() const final                         { return m_typeId; }

	virtual const char*            GetLabel() const final                          { return m_label.c_str(); }

	virtual void                   SetDescription(const char* szDescription) final { m_description = szDescription; }
	virtual const char*            GetDescription() const final                    { return m_description.c_str(); }

	virtual bool                   IsConst() const final                           { return m_isConst; }
	virtual bool                   IsPublic() const final                          { return m_isPublic; }

	virtual bool                   IsMemberVariable() const final                  { return (m_pParentTypeDesc != nullptr); }
	virtual ptrdiff_t              GetOffset() const final                         { return (IsMemberVariable() ? m_offset : std::numeric_limits<ptrdiff_t>::min()); }
	virtual void*                  GetPointer() const final                        { return (!IsMemberVariable() ? m_pAddress : nullptr); }

	virtual const ITypeDesc*       GetTypeDesc() const final;
	virtual const SSourceFileInfo& GetSourceInfo() const final { return m_sourceInfo; }
	// ~IVariableDesc

private:
	const CGuid      m_guid;
	const CTypeDesc* m_pParentTypeDesc;
	SSourceFileInfo  m_sourceInfo;

	string           m_label;
	string           m_description;

	union
	{
		ptrdiff_t m_offset;
		void*     m_pAddress;
	};

	CTypeId m_typeId;
	bool    m_isConst  : 1;
	bool    m_isPublic : 1;
};

inline CVariableDesc::CVariableDesc(void* pAddress, const char* szLabel, CTypeId typeId, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcInfo)
	: m_typeId(typeId)
	, m_pParentTypeDesc(nullptr)
	, m_pAddress(pAddress)
	, m_guid(guid)
	, m_label(szLabel)
	, m_isConst(isConst)
	, m_isPublic(isPublic)
	, m_sourceInfo(srcInfo)
{}

inline CVariableDesc::CVariableDesc(ptrdiff_t offset, const char* szLabel, const CTypeDesc& parentTypeDesc, CTypeId typeId, CGuid guid, bool isConst, bool isPublic, const SSourceFileInfo& srcInfo)
	: m_typeId(typeId)
	, m_pParentTypeDesc(&parentTypeDesc)
	, m_offset(offset)
	, m_guid(guid)
	, m_label(szLabel)
	, m_isConst(isConst)
	, m_isPublic(isPublic)
	, m_sourceInfo(srcInfo)
{}

} // ~Reflection namespace
} // ~Cry namespace
