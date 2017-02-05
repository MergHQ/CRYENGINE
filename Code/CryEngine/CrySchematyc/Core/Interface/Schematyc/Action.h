// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/IObject.h"
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
	CComponent*  pComponent = nullptr;
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

	inline void  PreInit(const SActionParams& params)
	{
		m_guid = params.guid;
		m_pObject = &params.object;
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

	inline CComponent* GetComponent() const
	{
		return m_pComponent;
	}

private:

	SGUID       m_guid;
	IObject*    m_pObject = nullptr;
	CComponent* m_pComponent = nullptr;
};

DECLARE_SHARED_POINTERS(CAction)

} // Schematyc
