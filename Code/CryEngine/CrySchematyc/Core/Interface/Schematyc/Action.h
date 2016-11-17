// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/Assert.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IObject;
// Forward declare classes
class CComponent;

struct SActionParams
{
	inline SActionParams(IObject& _object, void* _pProperties, CComponent* _pComponent)
		: object(_object)
		, pProperties(_pProperties)
		, pComponent(_pComponent)
	{}

	IObject&    object;
	void*       pProperties;
	CComponent* pComponent;
};

class CAction
{
public:

	virtual bool Init()
	{
		return true;
	}

	virtual void Start()    {}
	virtual void Stop()     {}
	virtual void Shutdown() {}

	virtual void PreInit(const SActionParams& params)
	{
		m_pObject = &params.object;
		m_pProperties = params.pProperties;
		m_pComponent = params.pComponent;
	}

	inline IObject& GetObject() const
	{
		SCHEMATYC_CORE_ASSERT(m_pObject);
		return *m_pObject;
	}

	inline void* GetProperties() const
	{
		return m_pProperties;
	}

	inline CComponent* GetComponent() const
	{
		return m_pComponent;
	}

private:

	IObject*    m_pObject = nullptr;
	void*       m_pProperties = nullptr;
	CComponent* m_pComponent = nullptr;
};

DECLARE_SHARED_POINTERS(CAction)
} // Schematyc
