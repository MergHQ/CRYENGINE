// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/IObject.h"
#include "Schematyc/Reflection/TypeDesc.h"
#include "Schematyc/Runtime/RuntimeParams.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{

// Forward declare interfaces.
struct IObject;
// Forward declare classes
class CComponent;

struct SActionParams
{
	inline SActionParams(const SGUID& _guid, IObject& _object)
		: guid(_guid)
		, object(_object)
	{}

	const SGUID& guid;
	IObject&     object;
	void*        pProperties = nullptr;
	CComponent*  pComponent = nullptr;
};

class CAction
{
public:

	virtual bool Init()
	{
		return true;
	}

	virtual void Start(const CRuntimeParams& params) {}
	virtual void Stop()                              {}
	virtual void Shutdown()                          {}

	inline void  PreInit(const SActionParams& params)
	{
		m_guid = params.guid;
		m_pObject = &params.object;
		m_pProperties = params.pProperties;
		m_pComponent = params.pComponent;
	}

	template<typename SIGNAL> inline void OutputSignal(const SIGNAL& signal)
	{
		GetObject().ProcessSignal(signal, m_guid);
	}

	inline const SGUID& GetGUID() const
	{
		return m_guid;
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

	SGUID       m_guid;
	IObject*    m_pObject = nullptr;
	void*       m_pProperties = nullptr;
	CComponent* m_pComponent = nullptr;
};

DECLARE_SHARED_POINTERS(CAction)

} // Schematyc
