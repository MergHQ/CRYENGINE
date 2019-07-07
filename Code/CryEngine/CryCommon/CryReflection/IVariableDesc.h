// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/IDescExtension.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

struct IVariableDesc : public IExtensibleDesc
{
	virtual ~IVariableDesc() {}

	virtual CGuid                  GetGuid() const = 0;
	virtual CTypeId                GetTypeId() const = 0;

	virtual const char*            GetLabel() const = 0;

	virtual void                   SetDescription(const char* szDescription) = 0;
	virtual const char*            GetDescription() const = 0;

	virtual bool                   IsConst() const = 0;
	virtual bool                   IsPublic() const = 0;

	virtual bool                   IsMemberVariable() const = 0;
	virtual ptrdiff_t              GetOffset() const = 0;
	virtual void*                  GetPointer() const = 0;

	virtual const ITypeDesc*       GetTypeDesc() const = 0;
	virtual const SSourceFileInfo& GetSourceInfo() const = 0;
};

template<typename VARIABLE_TYPE, typename OBJECT_TYPE>
class CMemberVariableOffset
{
public:
	CMemberVariableOffset(VARIABLE_TYPE OBJECT_TYPE::* pVariable)
	{
		m_offset = reinterpret_cast<uint8*>(&(reinterpret_cast<OBJECT_TYPE*>(1)->*pVariable)) - reinterpret_cast<uint8*>(1);
	}

	operator ptrdiff_t() const
	{
		return m_offset;
	}

private:
	ptrdiff_t m_offset;
};

} // ~Reflection namespace
} // ~Cry namespace
