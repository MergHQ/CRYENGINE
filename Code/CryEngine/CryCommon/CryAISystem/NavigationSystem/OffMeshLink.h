// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IOffMeshNavigationManager;

namespace MNM
{

//! Interface class for custom off-mesh link data
struct IOffMeshLink : public _i_reference_target_t
{
public:

	//! Used for acquiring navigation data of the link during the addition process.
	struct SNavigationData
	{
		NavigationMeshID meshId;
		TriangleID       startTriangleId;
		TriangleID       endTriangleId;
		AreaAnnotation   annotation;
	};

	virtual ~IOffMeshLink() {}

protected:

	IOffMeshLink(const CryGUID& linkTypeGuid, const EntityId entityId)
		: m_instanceId()
		, m_entityId(entityId)
		, m_typeGuid(linkTypeGuid)
	{
	}

	// Do not implement.
	IOffMeshLink() = delete;

public:
	//! Returns off-mesh link ID
	inline OffMeshLinkID GetLinkId() const                 { return m_instanceId; }

	//! Returns owner's entity ID
	inline EntityId      GetEntityIdForOffMeshLink() const { return m_entityId; }

	template<typename LinkDataClass>
	bool IsTypeOf() const
	{
		return m_typeGuid == LinkDataClass::GetGuid();
	}

	template<class LinkDataClass>
	LinkDataClass* CastTo()
	{
		return IsTypeOf<LinkDataClass>() ? static_cast<LinkDataClass*>(this) : nullptr;
	}

	template<class LinkDataClass>
	const LinkDataClass* CastTo() const
	{
		return IsTypeOf<LinkDataClass>() ? static_cast<const LinkDataClass*>(this) : nullptr;
	}

	//! Returns true if the off-mesh link can be used by requester entity in path-finding and fills the cost multiplier for traversing the link
	//! Warning: In the case of asynchronous path-finding, this function can be called from different threads. It is responsibility of the implementation code to make it thread safe.
	virtual bool          CanUse(const EntityId requesterEntityId, float* pCostMultiplier) const = 0;

	//! Returns start position of the off-mesh link
	virtual Vec3          GetStartPosition() const = 0;

	//! Returns end position of the off-mesh link
	virtual Vec3          GetEndPosition() const = 0;

private:
	friend struct ::IOffMeshNavigationManager;

	//! Sets off-mesh link ID. Called only by the IOffMeshNavigationManager when the link is created.
	inline void SetLinkId(OffMeshLinkID linkID) { m_instanceId = linkID; }

protected:

	OffMeshLinkID m_instanceId;
	EntityId      m_entityId;
	CryGUID       m_typeGuid;
};

TYPEDEF_AUTOPTR(IOffMeshLink);

} // namespace MNM
