// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ShootOp.h"

#include <CrySystem/XML/XMLAttrReader.h>
#include "PipeUser.h"

#include <CryAISystem/IMovementSystem.h>
#include <CryAISystem/MovementRequest.h>

ShootOp::ShootOp()
	: m_position(ZERO)
	, m_duration(0.0f)
	, m_shootAt(LocalSpacePosition)
	, m_fireMode(FIREMODE_OFF)
	, m_stance(STANCE_NULL)
{
}

ShootOp::ShootOp(const XmlNodeRef& node)
	: m_position(ZERO)
	, m_duration(0.0f)
	, m_shootAt(LocalSpacePosition)
	, m_fireMode(FIREMODE_OFF)
	, m_stance(STANCE_NULL)
{
	const ShootOpDictionary shootOpDictionary;
	const AgentDictionary agentDictionary;

	shootOpDictionary.shootAtXml.Get(node, "at", m_shootAt, true);
	agentDictionary.fireModeXml.Get(node, "fireMode", m_fireMode, true);
	node->getAttr("position", m_position);
	node->getAttr("duration", m_duration);
	s_xml.GetStance(node, "stance", m_stance);
}

ShootOp::ShootOp(const ShootOp& rhs)
	: m_position(rhs.m_position)
	, m_duration(rhs.m_duration)
	, m_shootAt(rhs.m_shootAt)
	, m_fireMode(rhs.m_fireMode)
	, m_stance(rhs.m_stance)
{
}

void ShootOp::Enter(CPipeUser& pipeUser)
{
	if (m_shootAt == LocalSpacePosition)
		SetupFireTargetForLocalPosition(pipeUser);
	else if (m_shootAt == ReferencePoint)
		SetupFireTargetForReferencePoint(pipeUser);

	pipeUser.SetFireMode(m_fireMode);

	if (m_stance != STANCE_NULL)
		pipeUser.m_State.bodystate = m_stance;

	MovementRequest movementRequest;
	movementRequest.entityID = pipeUser.GetEntityID();
	movementRequest.type = MovementRequest::Stop;
	gEnv->pAISystem->GetMovementSystem()->QueueRequest(movementRequest);

	m_timeWhenShootingShouldEnd = GetAISystem()->GetFrameStartTime() + CTimeValue(m_duration);
}

void ShootOp::Leave(CPipeUser& pipeUser)
{
	if (m_dummyTarget)
	{
		pipeUser.SetFireTarget(NILREF);
		m_dummyTarget.Release();
	}

	pipeUser.SetFireMode(FIREMODE_OFF);
}

void ShootOp::Update(CPipeUser& pipeUser)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);

	const CTimeValue& now = GetAISystem()->GetFrameStartTime();

	if (now >= m_timeWhenShootingShouldEnd)
		GoalOpSucceeded();
}

void ShootOp::Serialize(TSerialize serializer)
{
	serializer.BeginGroup("ShootOp");

	serializer.Value("position", m_position);
	serializer.Value("timeWhenShootingShouldEnd", m_timeWhenShootingShouldEnd);
	serializer.Value("duration", m_duration);
	serializer.EnumValue("shootAt", m_shootAt, ShootAtSerializationHelperFirst, ShootAtSerializationHelperLast);
	serializer.EnumValue("fireMode", m_fireMode, FIREMODE_SERIALIZATION_HELPER_FIRST, FIREMODE_SERIALIZATION_HELPER_LAST);
	m_dummyTarget.Serialize(serializer, "shootTarget");

	serializer.EndGroup();
}

void ShootOp::SetupFireTargetForLocalPosition(CPipeUser& pipeUser)
{
	// The user of this op has specified that the character
	// should fire at a predefined position in local-space.
	// Transform that position into world-space and store it
	// in an AI object. The pipe user will shoot at this.

	gAIEnv.pAIObjectManager->CreateDummyObject(m_dummyTarget);

	const Vec3 fireTarget = pipeUser.GetEntity()->GetWorldTM().TransformPoint(m_position);

	m_dummyTarget->SetPos(fireTarget);

	pipeUser.SetFireTarget(m_dummyTarget);
}

void ShootOp::SetupFireTargetForReferencePoint(CPipeUser& pipeUser)
{
	gAIEnv.pAIObjectManager->CreateDummyObject(m_dummyTarget);

	if (IAIObject* referencePoint = pipeUser.GetRefPoint())
	{
		m_dummyTarget->SetPos(referencePoint->GetPos());
	}
	else
	{
		gEnv->pLog->LogWarning("Agent '%s' is trying to shoot at the reference point but it hasn't been set.", pipeUser.GetName());
		m_dummyTarget->SetPos(ZERO);
	}

	pipeUser.SetFireTarget(m_dummyTarget);
}
