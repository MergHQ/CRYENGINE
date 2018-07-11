// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/IOffMeshNavigationManager.h>
#include <CryAISystem/INavigationSystem.h>

class CSmartObject;
class CSmartObjectClass;
struct SmartObjectHelper;

class CSmartObjectOffMeshNavigation 
	: public IOffMeshNavigationListener
	, public INavigationSystem::INavigationSystemListener
{
public:
	CSmartObjectOffMeshNavigation();
	~CSmartObjectOffMeshNavigation();

	void RegisterSmartObject(CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass);
	void UnregisterSmartObjectForAllClasses(CSmartObject* pSmartObject);

	bool IsObjectLinkedWithNavigationMesh(const EntityId objectId) const;

	void UpdateEditorDebugHelpers();

private:

	// IOffMeshNavigationListener
	virtual void OnOffMeshLinkGoingToBeRemoved(const MNM::OffMeshLinkID& linkID) override;
	virtual void OnOffMeshLinkGoingToBeRefreshed(MNM::LinkAdditionRequest& request) override;
	virtual void OnRefreshConnections(const NavigationMeshID meshID, const MNM::TileID tileID) override;
	// ~IOffMeshNavigationListener

	// INavigationSystem::INavigationSystemListener
	virtual void OnNavigationEvent(const INavigationSystem::ENavigationEvent event) override;
	// ~INavigationSystem::INavigationSystemListener

	bool ObjectRegistered(const EntityId objectId, const string& smartObjectClassName) const;
	void UnregisterSmartObject(CSmartObject* pSmartObject, const string& smartObjectClassName);

	// Tracking of objects registered
	// All registered objects are stored here, and some additional data
	// like to witch mesh they belong (only one), or bound triangles/tiles
	struct OffMeshLinkIDList
	{
		typedef std::vector<MNM::OffMeshLinkID> TLinkIDList;

		TLinkIDList&       GetLinkIDList() { return offMeshLinkIDList; }
		const TLinkIDList& GetLinkIDList() const { return offMeshLinkIDList; }

		void               OnLinkAdditionRequestForSmartObjectServiced(const MNM::SOffMeshOperationCallbackData& callbackData)
		{
			if (callbackData.operationSucceeded)
			{
				assert(callbackData.linkID != MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID);
				offMeshLinkIDList.push_back(callbackData.linkID);
			}
		}
		void               OnLinkRepairRequestForSmartObjectServiced(const MNM::SOffMeshOperationCallbackData& callbackData)
		{
			if (!callbackData.operationSucceeded)
			{
				auto it = std::find(offMeshLinkIDList.begin(), offMeshLinkIDList.end(), callbackData.linkID);
				assert(it != offMeshLinkIDList.end());
				if (it != offMeshLinkIDList.end())
				{
					offMeshLinkIDList.erase(it);
				}
			}
		}

	private:
		TLinkIDList offMeshLinkIDList;
	};

	typedef stl::STLPoolAllocator<std::pair<const uint32, OffMeshLinkIDList>, stl::PoolAllocatorSynchronizationSinglethreaded> TSOClassInfoAllocator;
	typedef std::map<uint32, OffMeshLinkIDList, std::less<uint32>, TSOClassInfoAllocator>             TSOClassInfo;
	typedef std::map<EntityId, TSOClassInfo>                                                          TRegisteredObjects;

	OffMeshLinkIDList* GetClassInfoFromLinkInfo(const MNM::OffMeshLinkPtr& linkInfo);

	TRegisteredObjects m_registeredObjects;
};

//////////////////////////////////////////////////////////////////////////
// OffMeshLink_SmartObject
//////////////////////////////////////////////////////////////////////////

struct OffMeshLink_SmartObject : public MNM::OffMeshLink
{
	OffMeshLink_SmartObject()
		: MNM::OffMeshLink(eLinkType_SmartObject, 0)
		, m_pSmartObject(nullptr)
		, m_pSmartObjectClass(nullptr)
		, m_pFromHelper(nullptr)
		, m_pToHelper(nullptr)
	{}

	OffMeshLink_SmartObject(const EntityId objectId, CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass, SmartObjectHelper* pFromHelper, SmartObjectHelper* pToHelper)
		: MNM::OffMeshLink(eLinkType_SmartObject, objectId)
		, m_pSmartObject(pSmartObject)
		, m_pSmartObjectClass(pSmartObjectClass)
		, m_pFromHelper(pFromHelper)
		, m_pToHelper(pToHelper)
	{}

	virtual ~OffMeshLink_SmartObject() override {}

	virtual MNM::OffMeshLink* Clone() const override
	{
		return new OffMeshLink_SmartObject(GetEntityIdForOffMeshLink(), m_pSmartObject, m_pSmartObjectClass, m_pFromHelper, m_pToHelper);
	}

	virtual Vec3 GetStartPosition() const override;
	virtual Vec3 GetEndPosition() const override;
	virtual bool CanUse(const IEntity* pRequester, float* costMultiplier) const override;

	static LinkType GetType() { return eLinkType_SmartObject; }

	CSmartObject*      m_pSmartObject;
	CSmartObjectClass* m_pSmartObjectClass;
	SmartObjectHelper* m_pFromHelper;
	SmartObjectHelper* m_pToHelper;
};
