// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Group.h"
#include "GroupManager.h"
#include "ObjectContainer.h"
#include "PipeUser.h"

Group::TargetInfos Group::s_targetInfos;

Group::Group()
	: m_targetThreat(AITHREAT_NONE)
	, m_targetType(AITARGET_NONE)
	, m_prevTargetType(AITARGET_NONE)
	, m_targetLocation(ZERO)
	, m_groupID(-1)
{
}

Group::Group(const GroupID& groupID)
	: m_targetThreat(AITHREAT_NONE)
	, m_targetType(AITARGET_NONE)
	, m_prevTargetType(AITARGET_NONE)
	, m_targetLocation(ZERO)
	, m_groupID(groupID)
{
	m_proxy.reset(GetAISystem()->GetGroupProxyFactory()->CreateGroupProxy(groupID));
}

Group::~Group()
{
}

const Group::Members& Group::GetMembers() const
{
	return m_members;
}

uint32 Group::GetMemberCount() const
{
	return m_members.size();
}

void Group::AddMember(tAIObjectID objectID)
{
#if defined(USE_CRY_ASSERT)
	std::pair<Group::Members::iterator, bool> miresult = m_members.insert(objectID);
	assert(miresult.second);
#else
	m_members.insert(objectID);
#endif

	m_proxy->MemberAdded(objectID);
}

void Group::RemoveMember(tAIObjectID objectID)
{
	m_members.erase(objectID);

	m_proxy->MemberRemoved(objectID);
}

void Group::Reset(EObjectResetType type)
{
	m_proxy->Reset(type);

	m_target.Release();
	m_targetThreat = AITHREAT_NONE;
	m_targetType = AITARGET_NONE;
	m_prevTargetType = AITARGET_NONE;
	m_targetLocation.zero();
}

void Group::Update(float updateTime)
{
	while (!m_notifications.empty())
	{
		QueuedNotification& notification = m_notifications.front();

		m_proxy->Notify(notification.ID, notification.pSignal);

		m_notifications.pop_front();
	}

	// This update call on the behavior is not needed for now /Mario
	//m_proxy->Notify(0, 0, "Update");

	Group::Members::const_iterator mit = m_members.begin();
	Group::Members::const_iterator mend = m_members.end();

	s_targetInfos.clear();
	s_targetInfos.reserve(8);

	for (; mit != mend; ++mit)
	{
		IAIObject* aiObject = gAIEnv.pObjectContainer->GetAIObject(*mit);
		CPipeUser* actor = aiObject ? aiObject->CastToCPipeUser() : 0;

		if (!actor || !actor->IsEnabled() || actor->IsPaused())
			continue;

		if (IAIObject* target = actor->GetAttentionTarget())
		{
			CAIObject* targetObject = static_cast<CAIObject*>(target);
			tAIObjectID targetID = 0;
			uint32 targetThreat = actor->GetAttentionTargetThreat();
			uint32 targetType = actor->GetAttentionTargetType();
			uint32 targetObjType = targetObject->GetType();

			if (targetObjType != AIOBJECT_DUMMY)
				targetID = target->GetAIObjectID();
			else if (CAIObject* association = targetObject->GetAssociation().GetAIObject())
				targetID = association->GetAIObjectID();

			if (actor->IsPersonallyHostile(targetID) &&
			    (gAIEnv.pFactionMap->GetReaction(actor->GetFactionID(), target->GetFactionID()) > IFactionMap::Hostile))
				continue;

			if (actor->GetState().bTargetIsGroupTarget)
				continue;

			uint32 infoCount = s_targetInfos.size();

			uint32 i = 0;
			for (; i < infoCount; ++i)
			{
				TargetInfo& info = s_targetInfos[i];
				if (info.targetID == targetID)
				{
					++info.occurences;
					info.position += targetObject->GetPos();

					if (info.threat < targetThreat)
						info.threat = targetThreat;

					if (info.type < targetType)
						info.type = targetType;

					break;
				}
			}

			if (i == infoCount)
				s_targetInfos.push_back(TargetInfo(targetID, targetThreat, targetType, targetObject->GetPos()));
		}
	}

	if (!s_targetInfos.empty())
	{
		std::sort(s_targetInfos.begin(), s_targetInfos.end());

		TargetInfo& info = s_targetInfos.back();

		m_targetThreat = (EAITargetThreat)info.threat;
		m_targetType = (EAITargetType)info.type;
		m_targetLocation = info.position / (float)info.occurences;

		if (m_target.IsNil() || !m_target.GetAIObject())
			gAIEnv.pAIObjectManager->CreateDummyObject(m_target, "");

		CAIObject* targetObj = m_target.GetAIObject();
		CWeakRef<CAIObject> assocRef = targetObj->GetAssociation();
		tAIObjectID currentID = assocRef.GetObjectID();

		if (currentID != info.targetID)
		{
			if (IAIObject* groupTarget = gAIEnv.pObjectContainer->GetAIObject(info.targetID))
			{
				char targetName[256];
				cry_sprintf(targetName, "Group Target [%s]", groupTarget->GetName());
				m_target->SetName(targetName);
				m_target->SetEntityID(groupTarget->GetEntityID());
			}
		}

		m_target->SetAssociation(gAIEnv.pObjectContainer->GetWeakRef(info.targetID));
		m_target->SetPos(m_targetLocation);
	}


	if (m_targetType != m_prevTargetType)
	{
		const AISignals::ISignalDescription* pSignalDescription = nullptr;
		switch (m_targetType)
		{
		case AITARGET_NONE:
			pSignalDescription = &GetAISystem()->GetSignalManager()->GetBuiltInSignalDescriptions().GetOnGroupTargetNone();
			break;
		case AITARGET_SOUND:
			pSignalDescription = &GetAISystem()->GetSignalManager()->GetBuiltInSignalDescriptions().GetOnGroupTargetSound();
			break;
		case AITARGET_MEMORY:
			pSignalDescription = &GetAISystem()->GetSignalManager()->GetBuiltInSignalDescriptions().GetOnGroupTargetMemory();
			break;
		case AITARGET_VISUAL:
			pSignalDescription = &GetAISystem()->GetSignalManager()->GetBuiltInSignalDescriptions().GetOnGroupTargetVisual();
			break;
		default:
			break;
		}

		if (pSignalDescription && !pSignalDescription->IsNone())
		{
			const AISignals::SignalSharedPtr pSignal = GetAISystem()->GetSignalManager()->CreateSignal(AISIGNAL_INCLUDE_DISABLED, *pSignalDescription);
			gAIEnv.pGroupManager->NotifyGroup(m_groupID, pSignal);
		}

		m_prevTargetType = m_targetType;
	}

	m_targetType = std::min(AITARGET_MEMORY, m_targetType);
}

bool Group::Empty() const
{
	return m_members.empty();
}

void Group::Swap(Group& other)
{
	std::swap(m_target, other.m_target);
	std::swap(m_targetThreat, other.m_targetThreat);
	std::swap(m_targetType, other.m_targetType);
	std::swap(m_targetLocation, other.m_targetLocation);
	std::swap(m_groupID, other.m_groupID);

	m_proxy.swap(other.m_proxy);
	m_members.swap(other.m_members);
}

const CCountedRef<CAIObject>& Group::GetTarget() const
{
	return m_target;
}

EAITargetThreat Group::GetTargetThreat() const
{
	return m_targetThreat;
}

EAITargetType Group::GetTargetType() const
{
	return m_targetType;
}

const Vec3& Group::GetTargetLocation() const
{
	return m_targetLocation;
}

void Group::Notify(const NotificationID& notificationID, AISignals::SignalSharedPtr pSignal)
{
	m_notifications.push_back(QueuedNotification(notificationID, pSignal));
}

void Group::Serialize(TSerialize ser)
{
	ser.Value("GroupID", m_groupID);

	if (ser.IsReading())
		Group(m_groupID).Swap(*this);

	ser.EnumValue("TargetThreat", m_targetThreat, AITHREAT_NONE, AITHREAT_LAST);
	ser.EnumValue("TargetType", m_targetType, AITARGET_NONE, AITARGET_LAST);
	ser.Value("TargetLocation", m_targetLocation);
	ser.Value("Members", m_members);
	ser.Value("Target", m_target);
	ser.Value("Notifications", m_notifications);
	m_proxy->Serialize(ser);
}
