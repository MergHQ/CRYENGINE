// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DescExtension.h"

#include <CryReflection/ITypeDesc.h>
#include <CryReflection/IFunctionDesc.h>

namespace Cry {
namespace Reflection {

class CFunctionParameterDesc : public IFunctionParameterDesc
{
public:
	CFunctionParameterDesc(const SFunctionParameterDesc& paramDesc)
		: m_flags(paramDesc.flags)
		, m_typeId(paramDesc.typeId)
		, m_index(paramDesc.index)
	{}

	// IFunctionParameterDesc
	virtual CTypeId            GetTypeId() const override                  { return m_typeId; }
	virtual uint32             GetIndex() const override                   { return m_index; }

	virtual FunctionParamFlags GetFlags() const override                   { return m_flags; }
	virtual void               SetFlags(FunctionParamFlags flags) override { m_flags = flags; }

	virtual bool               IsConst() const override                    { return m_flags.Check(EFunctionParamFlags::IsConst); }
	virtual bool               IsReference() const override                { return m_flags.Check(EFunctionParamFlags::IsReference); }
	// ~IFunctionParameterDesc

private:
	FunctionParamFlags m_flags;
	CTypeId            m_typeId;
	uint32             m_index;
};

class CFunctionDesc : public CExtensibleDesc<IFunctionDesc>
{
public:
	CFunctionDesc(CTypeId returnTypeId, const CFunctionDelegate& functionDelegate, const ParamArray& params, const char* szFullQualifiedName, const char* szLabel, const CryGUID& guid);
	CFunctionDesc(const ITypeDesc* pObjectType, CTypeId returnTypeId, const CFunctionDelegate& functionDelegate, const ParamArray& params, const char* szLabel, const CryGUID& guid);

	// IFunctionDesc
	virtual CGuid                         GetGuid() const final                           { return m_guid; }
	virtual FunctionIndex                 GetIndex() const final                          { return IsMemberFunction() ? FunctionIndex() : m_index; }

	virtual const char*                   GetFullQualifiedName() const final              { return m_fullQualifiedName.c_str(); }
	virtual const char*                   GetLabel() const final                          { return m_label.c_str(); }
	virtual void                          SetLabel(const char* szLabel)     final         { m_label = szLabel; }

	virtual const char*                   GetDescription() const final                    { return m_description.c_str(); }
	virtual void                          SetDescription(const char* szDescription) final { m_description = szDescription; }

	virtual bool                          IsMemberFunction() const final                  { return (m_pObjectType != nullptr); }

	virtual const CFunctionDelegate&      GetFunctionDelegate() const final               { return m_delegate; }

	virtual const ITypeDesc*              GetObjectTypeDesc() const final    { return m_pObjectType; }
	virtual CTypeId                       GetReturnTypeId() const final      { return m_returnTypeId; }

	virtual size_t                        GetParamCount() const final        { return m_params.size(); }
	virtual const IFunctionParameterDesc* GetParam(size_t index) const final { return (index < m_params.size() ? &m_params[index] : nullptr); }

	virtual const SSourceFileInfo&        GetSourceInfo() const final        { return m_sourcePos; }
	// ~IFunctionDesc

private:
	friend class CCoreRegistry;

private:
	CryGUID                             m_guid;
	FunctionIndex                       m_index;

	CFunctionDelegate                   m_delegate;

	string                              m_fullQualifiedName;
	string                              m_label;
	string                              m_description;

	const ITypeDesc*                    m_pObjectType;
	CTypeId                             m_returnTypeId;
	std::vector<CFunctionParameterDesc> m_params;

	bool                                m_isConstFunction;

	SSourceFileInfo                     m_sourcePos;
};

inline CFunctionDesc::CFunctionDesc(CTypeId returnTypeId, const CFunctionDelegate& functionDelegate, const ParamArray& params, const char* szFullQualifiedName, const char* szLabel, const CryGUID& guid)
	: m_fullQualifiedName(szFullQualifiedName)
	, m_pObjectType(nullptr)
	, m_returnTypeId(returnTypeId)
	, m_delegate(functionDelegate)
	, m_label(szLabel)
	, m_isConstFunction(false)
	, m_guid(guid)
{
	for (const SFunctionParameterDesc& desc : params)
	{
		m_params.emplace_back(desc);
	}
}

inline CFunctionDesc::CFunctionDesc(const ITypeDesc* pObjectType, CTypeId returnTypeId, const CFunctionDelegate& functionDelegate, const ParamArray& params, const char* szLabel, const CryGUID& guid)
	: m_fullQualifiedName(pObjectType->GetFullQualifiedName())
	, m_pObjectType(pObjectType)
	, m_returnTypeId(returnTypeId)
	, m_delegate(functionDelegate)
	, m_label(szLabel)
	, m_isConstFunction(false)
	, m_guid(guid)
{
	for (const SFunctionParameterDesc& desc : params)
	{
		m_params.emplace_back(desc);
	}
}

} // ~Reflection namespace
} // ~Cry namespace
