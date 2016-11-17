// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvElement;

enum class EEnvElementType
{
	Root,
	Module,
	DataType,
	Signal,
	Function,
	Class,
	Interface,
	InterfaceFunction,
	Component,
	Action
};

enum class EEnvElementFlags
{
	None       = 0,
	Deprecated = BIT(1)
};

typedef CEnumFlags<EEnvElementFlags>                EnvElementFlags;

typedef CDelegate<EVisitStatus(const IEnvElement&)> EnvElementConstVisitor;

// Environment element interface.
// N.B. Do not inherit from this class directly but instead use IEnvElementBase.
struct IEnvElement
{
	virtual ~IEnvElement() {}

	virtual EEnvElementType    GetElementType() const = 0;
	virtual EnvElementFlags    GetElementFlags() const = 0;
	virtual SGUID              GetGUID() const = 0;
	virtual const char*        GetName() const = 0;
	virtual SSourceFileInfo    GetSourceFileInfo() const = 0;
	virtual const char*        GetAuthor() const = 0;
	virtual const char*        GetDescription() const = 0;
	virtual const char*        GetWikiLink() const = 0;

	virtual bool               IsValidScope(IEnvElement& scope) const = 0;

	virtual bool               AttachChild(IEnvElement& child) = 0;
	virtual void               DetachChild(IEnvElement& child) = 0;
	virtual void               SetParent(IEnvElement* pParent) = 0;
	virtual void               SetPrevSibling(IEnvElement* pPrevSibling) = 0;
	virtual void               SetNextSibling(IEnvElement* pNextSibling) = 0;

	virtual IEnvElement*       GetParent() = 0;
	virtual const IEnvElement* GetParent() const = 0;
	virtual IEnvElement*       GetFirstChild() = 0;
	virtual const IEnvElement* GetFirstChild() const = 0;
	virtual IEnvElement*       GetLastChild() = 0;
	virtual const IEnvElement* GetLastChild() const = 0;
	virtual IEnvElement*       GetPrevSibling() = 0;
	virtual const IEnvElement* GetPrevSibling() const = 0;
	virtual IEnvElement*       GetNextSibling() = 0;
	virtual const IEnvElement* GetNextSibling() const = 0;

	virtual EVisitStatus       VisitChildren(const EnvElementConstVisitor& visitor) const = 0;
};

template<EEnvElementType ELEMENT_TYPE> struct IEnvElementBase : public IEnvElement
{
	static const EEnvElementType ElementType = ELEMENT_TYPE;

	virtual EEnvElementType GetElementType() const override
	{
		return ElementType;
	}
};

template<typename TYPE> inline TYPE& DynamicCast(IEnvElement& envElement)
{
	SCHEMATYC_CORE_ASSERT(envElement.GetElementType() == TYPE::ElementType);
	return static_cast<TYPE&>(envElement);
}

template<typename TYPE> inline const TYPE& DynamicCast(const IEnvElement& envElement)
{
	SCHEMATYC_CORE_ASSERT(envElement.GetElementType() == TYPE::ElementType);
	return static_cast<const TYPE&>(envElement);
}

template<typename TYPE> inline TYPE* DynamicCast(IEnvElement* pEnvElement)
{
	return pEnvElement && (pEnvElement->GetElementType() == TYPE::ElementType) ? static_cast<TYPE*>(pEnvElement) : nullptr;
}

template<typename TYPE> inline const TYPE* DynamicCast(const IEnvElement* pEnvElement)
{
	return pEnvElement && (pEnvElement->GetElementType() == TYPE::ElementType) ? static_cast<const TYPE*>(pEnvElement) : nullptr;
}
} // Schematyc
