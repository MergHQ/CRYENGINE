// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

#include "Schematyc/Utils/Assert.h"
#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Network/INetworkSpawnParams.h"

// Forward declare structures.
struct SRendParams;
struct SRenderingPassInfo;

namespace Schematyc
{
// Forward declare interfaces.
struct IComponentPreviewer;
struct IObject;
// Forward declare classes
class CComponent;
class CTransform;

struct SComponentParams
{
	inline SComponentParams(IObject& _object, CTransform& _transform)
		: object(_object)
		, transform(_transform)
	{}

	IObject&               object;
	CTransform&            transform;
	void*                  pProperties = nullptr;
	IComponentPreviewer*   pPreviewer = nullptr;
	CComponent*            pParent = nullptr;
	INetworkSpawnParamsPtr pNetworkSpawnParams; // #SchematycTODO : Should this also be a raw pointer?
};

static constexpr int EmptySlot = -1;

class CComponent
{
public:

	virtual ~CComponent() {}

	virtual bool Init()
	{
		return true;
	}

	virtual void Run(ESimulationMode simulationMode) {}

	virtual void Shutdown()                          {}

	virtual void PreInit(const SComponentParams& params)
	{
		m_pObject = &params.object;
		m_pTransform = &params.transform;
		m_pProperties = params.pProperties;
		m_pPreviewer = params.pPreviewer;
		m_pParent = params.pParent;
	}

	virtual INetworkSpawnParamsPtr GetNetworkSpawnParams() const
	{
		return INetworkSpawnParamsPtr();
	}

	virtual int GetSlot() const // #SchematycTODO : This name is super generic. Rename GetEntitySlot() or create CEntityComponent class?
	{
		return EmptySlot;
	}

	inline IObject& GetObject() const
	{
		SCHEMATYC_CORE_ASSERT(m_pObject);
		return *m_pObject;
	}

	inline CTransform& GetTransform() const
	{
		SCHEMATYC_CORE_ASSERT(m_pTransform);
		return *m_pTransform;
	}

	inline void* GetProperties() const
	{
		return m_pProperties;
	}

	inline IComponentPreviewer* GetPreviewer() const
	{
		return m_pPreviewer;
	}

	inline CComponent* GetParent() const
	{
		return m_pParent;
	}

private:

	IObject*             m_pObject = nullptr;
	CTransform*          m_pTransform = nullptr;
	void*                m_pProperties = nullptr;
	IComponentPreviewer* m_pPreviewer = nullptr;
	CComponent*          m_pParent = nullptr;
};

struct IComponentPreviewer
{
	virtual ~IComponentPreviewer() {}

	virtual void SerializeProperties(Serialization::IArchive& archive) = 0;
	virtual void Render(const IObject& object, const CComponent& component, const SRendParams& params, const SRenderingPassInfo& passInfo) const = 0; // #SchematycTODO : Pass SRenderContext instead of SRendParams and SRenderingPassInfo?
};
} // Schematyc
