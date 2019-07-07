// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/IOffMeshNavigationManager.h>
#include <CryAISystem/INavigationSystem.h>

class CSmartObject;
class CSmartObjectClass;
struct SmartObjectHelper;

class CSmartObjectOffMeshNavigation : public INavigationSystem::INavigationSystemListener
{
public:
	CSmartObjectOffMeshNavigation();
	~CSmartObjectOffMeshNavigation();

	void RegisterSmartObject(CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass);
	void UnregisterSmartObjectForAllClasses(CSmartObject* pSmartObject);

	bool IsObjectLinkedWithNavigationMesh(const EntityId objectId) const;

	void UpdateEditorDebugHelpers();

private:

	void OnNavMeshChanged(NavigationAgentTypeID navAgentId, NavigationMeshID navMeshId, MNM::TileID tileId);

	MNM::EOffMeshLinkAdditionResult OnLinkDataRequested(const NavigationAgentTypeID agentTypeId, MNM::IOffMeshLink* pLinkData, MNM::IOffMeshLink::SNavigationData& linkNavigationData) const;
	void OnOffmeshLinkRemoved(const MNM::OffMeshLinkID linkId, const NavigationMeshID meshId, const MNM::EOffMeshLinkRemovalReason reason);

	// INavigationSystem::INavigationSystemListener
	virtual void OnNavigationEvent(const INavigationSystem::ENavigationEvent event) override;
	// ~INavigationSystem::INavigationSystemListener

	bool ObjectRegistered(const EntityId objectId, const string& smartObjectClassName) const;
	void UnregisterSmartObject(CSmartObject* pSmartObject, const string& smartObjectClassName);

	// Tracking of objects registered
	// All registered objects are stored here, and some additional data
	// like to witch mesh they belong (only one), or bound triangles/tiles
protected:

	struct OffMeshLinkIDList
	{
		typedef std::vector<MNM::OffMeshLinkID> TLinkIDList;

		TLinkIDList&       GetLinkIDList()       { return offMeshLinkIDList; }
		const TLinkIDList& GetLinkIDList() const { return offMeshLinkIDList; }

		void AddRequestededLinkId(const MNM::OffMeshLinkID linkId) { requestedOffmeshLinkIDList.push_back(linkId); }
		const TLinkIDList& GetRequestedIDList() const { return requestedOffmeshLinkIDList; }

		void               OnLinkAdditionObjectServiced(MNM::OffMeshLinkID linkId, const MNM::SOffMeshAdditionCallbackData& callbackData)
		{
			CRY_ASSERT(linkId.IsValid());

			stl::find_and_erase(requestedOffmeshLinkIDList, linkId);

			if (callbackData.result == MNM::EOffMeshLinkAdditionResult::Success)
			{
				offMeshLinkIDList.push_back(linkId);
			}
			else
			{
				stl::find_and_erase(offMeshLinkIDList, linkId);
			}
		}

	private:
		TLinkIDList offMeshLinkIDList;
		TLinkIDList requestedOffmeshLinkIDList;
	};

	typedef stl::STLPoolAllocator<std::pair<const uint32, OffMeshLinkIDList>, stl::PoolAllocatorSynchronizationSinglethreaded> TSOClassInfoAllocator;
	typedef std::map<uint32, OffMeshLinkIDList, std::less<uint32>, TSOClassInfoAllocator>                                      TSOClassInfo;
	typedef std::map<EntityId, TSOClassInfo>                                                                                   TRegisteredObjects;

	TRegisteredObjects m_registeredObjects;
};

//////////////////////////////////////////////////////////////////////////
// OffMeshLink_SmartObject
//////////////////////////////////////////////////////////////////////////

struct OffMeshLink_SmartObject : public MNM::IOffMeshLink
{	
	OffMeshLink_SmartObject()
		: MNM::IOffMeshLink(GetGuid(), 0)
		, m_pSmartObject(nullptr)
		, m_pSmartObjectClass(nullptr)
		, m_pFromHelper(nullptr)
		, m_pToHelper(nullptr)
	{}

	OffMeshLink_SmartObject(const EntityId objectId, CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper)
		: MNM::IOffMeshLink(GetGuid(), objectId)
		, m_pSmartObject(pSmartObject)
		, m_pSmartObjectClass(pSmartObjectClass)
		, m_pFromHelper(pFromHelper)
		, m_pToHelper(pToHelper)
	{}

	virtual ~OffMeshLink_SmartObject() override {}

	static const CryGUID& GetGuid()
	{
		static CryGUID id = "1B9D4053-AC3A-43A9-B470-18914A5F132E"_cry_guid;
		return id;
	}
	virtual Vec3    GetStartPosition() const override;
	virtual Vec3    GetEndPosition() const override;
	virtual bool    CanUse(const EntityId requesterEntityId, float* pCostMultiplier) const override;

	MNM::EOffMeshLinkAdditionResult GetNavigationLinkData(const NavigationAgentTypeID agentTypeId, SNavigationData& navigationLinkData) const;

	CSmartObject*      m_pSmartObject;
	CSmartObjectClass* m_pSmartObjectClass;
	SmartObjectHelper* m_pFromHelper;
	SmartObjectHelper* m_pToHelper;
};
