// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//! \cond INTERNAL

#pragma once

#include <CrySystem/XML/XMLAttrReader.h>
#include <CryAISystem/IAgent.h>

//! Defines how an agent should move to the destination.
//! Moving to cover, running, crouching and so on.
class MovementStyle
{
public:
	// When these are changed, ReadFromXml should be updated as well.
	enum Stance
	{
		Relaxed,
		Alerted,
		Stand,
		Crouch
	};

	enum Speed
	{
		Walk,
		Run,
		Sprint
	};

public:
	MovementStyle()
		: m_stance(Stand)
		, m_speed(Run)
		, m_speedLiteral(0.0f)
		, m_hasSpeedLiteral(false)
		, m_bodyOrientationMode(HalfwayTowardsAimOrLook)
		, m_movingToCover(false)
		, m_movingAlongDesignedPath(false)
		, m_turnTowardsMovementDirectionBeforeMoving(false)
		, m_strafe(false)
		, m_hasExactPositioningRequest(false)
		, m_glanceInMovementDirection(false)
	{
	}

	void ConstructDictionariesIfNotAlreadyDone();
	void ReadFromXml(const XmlNodeRef& node);
	void WriteToXml(const XmlNodeRef& xml);

	void SetStance(const Stance stance)                                  { m_stance = stance; }
	void SetSpeed(const Speed speed)                                     { m_speed = speed; }
	void SetSpeedLiteral(float speedLiteral)                             { m_speedLiteral = speedLiteral; m_hasSpeedLiteral = true; }
	void SetBodyOrientationMode(const EBodyOrientationMode orientation)  { m_bodyOrientationMode = orientation; }
	void SetMovingToCover(const bool movingToCover)                      { m_movingToCover = movingToCover; }
	void SetTurnTowardsMovementDirectionBeforeMoving(const bool enabled) { m_turnTowardsMovementDirectionBeforeMoving = enabled; }
	void SetShouldStrafe(const bool enabled)                             { m_strafe = enabled; }
	void SetShouldGlanceInMovementDirection(const bool enabled)          { m_glanceInMovementDirection = enabled; }

	void SetMovingAlongDesignedPath(const bool movingAlongDesignedPath)  { m_movingAlongDesignedPath = movingAlongDesignedPath; }
	void SetExactPositioningRequest(const SAIActorTargetRequest* const pExactPositioningRequest)
	{
		if (pExactPositioningRequest != 0)
		{
			m_hasExactPositioningRequest = true;
			m_exactPositioningRequest = *pExactPositioningRequest;
		}
		else
		{
			m_hasExactPositioningRequest = false;
		}
	}

	Stance                       GetStance() const                                      { return m_stance; }
	Speed                        GetSpeed() const                                       { return m_speed; }
	bool                         HasSpeedLiteral() const                                { return m_hasSpeedLiteral; }
	float                        GetSpeedLiteral() const                                { return m_speedLiteral; }
	EBodyOrientationMode         GetBodyOrientationMode() const                         { return m_bodyOrientationMode; }
	bool                         IsMovingToCover() const                                { return m_movingToCover; }
	bool                         IsMovingAlongDesignedPath() const                      { return m_movingAlongDesignedPath; }
	bool                         ShouldTurnTowardsMovementDirectionBeforeMoving() const { return m_turnTowardsMovementDirectionBeforeMoving; }
	bool                         ShouldStrafe() const                                   { return m_strafe; }
	bool                         ShouldGlanceInMovementDirection() const                { return m_glanceInMovementDirection; }
	const SAIActorTargetRequest* GetExactPositioningRequest() const                     { return m_hasExactPositioningRequest ? &m_exactPositioningRequest : nullptr; }

private:
	Stance                m_stance;
	Speed                 m_speed;
	float                 m_speedLiteral;
	bool                  m_hasSpeedLiteral;
	EBodyOrientationMode  m_bodyOrientationMode;
	SAIActorTargetRequest m_exactPositioningRequest;
	bool                  m_movingToCover;
	bool                  m_movingAlongDesignedPath;
	bool                  m_turnTowardsMovementDirectionBeforeMoving;
	bool                  m_strafe;
	bool                  m_hasExactPositioningRequest;
	bool                  m_glanceInMovementDirection;
};

struct MovementStyleDictionaryCollection
{
	MovementStyleDictionaryCollection();

	CXMLAttrReader<MovementStyle::Speed>  speedsXml;
	Serialization::StringList             speedsSerialization;

	CXMLAttrReader<MovementStyle::Stance> stancesXml;
	Serialization::StringList             stancesSerialization;

	CXMLAttrReader<bool>                  booleans;
};

inline MovementStyleDictionaryCollection::MovementStyleDictionaryCollection()
{
	speedsXml.Reserve(3);
	speedsXml.Add("Walk", MovementStyle::Walk);
	speedsXml.Add("Run", MovementStyle::Run);
	speedsXml.Add("Sprint", MovementStyle::Sprint);

	speedsSerialization.reserve(3);
	speedsSerialization.push_back("Walk");
	speedsSerialization.push_back("Run");
	speedsSerialization.push_back("Sprint");

	stancesXml.Reserve(4);
	stancesXml.Add("Relaxed", MovementStyle::Relaxed);
	stancesXml.Add("Alerted", MovementStyle::Alerted);
	stancesXml.Add("Stand", MovementStyle::Stand);
	stancesXml.Add("Crouch", MovementStyle::Crouch);

	stancesSerialization.reserve(4);
	stancesSerialization.push_back("Relaxed");
	stancesSerialization.push_back("Alerted");
	stancesSerialization.push_back("Stand");
	stancesSerialization.push_back("Crouch");

	booleans.Reserve(4);
	booleans.Add("true", true);
	booleans.Add("false", false);
	booleans.Add("1", true);
	booleans.Add("0", false);
}

inline void MovementStyle::ReadFromXml(const XmlNodeRef& node)
{
	const MovementStyleDictionaryCollection movementStyleDictionaryCollection;
	const AgentDictionary agentDictionary;

	movementStyleDictionaryCollection.speedsXml.Get(node, "speed", m_speed, true);
	movementStyleDictionaryCollection.stancesXml.Get(node, "stance", m_stance);
	agentDictionary.bodyOrientationsXml.Get(node, "bodyOrientation", m_bodyOrientationMode);
	movementStyleDictionaryCollection.booleans.Get(node, "moveToCover", m_movingToCover);
	movementStyleDictionaryCollection.booleans.Get(node, "turnTowardsMovementDirectionBeforeMoving", m_turnTowardsMovementDirectionBeforeMoving);
	movementStyleDictionaryCollection.booleans.Get(node, "strafe", m_strafe);
	movementStyleDictionaryCollection.booleans.Get(node, "glanceInMovementDirection", m_glanceInMovementDirection);
}

inline void MovementStyle::WriteToXml(const XmlNodeRef& node)
{
	const MovementStyleDictionaryCollection movementStyleDictionaryCollection;

	node->setAttr("speed", Serialization::getEnumLabel<MovementStyle::Speed>(m_speed));
	node->setAttr("stance", Serialization::getEnumLabel<MovementStyle::Stance>(m_stance));
	node->setAttr("bodyOrientation", Serialization::getEnumLabel<EBodyOrientationMode>(m_bodyOrientationMode));
	node->setAttr("moveToCover", m_movingToCover);
	node->setAttr("turnTowardsMovementDirectionBeforeMoving", m_turnTowardsMovementDirectionBeforeMoving);
	node->setAttr("strafe", m_strafe);
	node->setAttr("glanceInMovementDirection", m_glanceInMovementDirection);
}

#ifndef SWIG
SERIALIZATION_ENUM_BEGIN_NESTED(MovementStyle, Stance, "Stance")
SERIALIZATION_ENUM(MovementStyle::Stance::Relaxed, "relaxed", "Relaxed")
SERIALIZATION_ENUM(MovementStyle::Stance::Alerted, "alerted", "Alerted")
SERIALIZATION_ENUM(MovementStyle::Stance::Stand, "stand", "Stand")
SERIALIZATION_ENUM(MovementStyle::Stance::Crouch, "crouch", "Crouch")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(MovementStyle, Speed, "Speed")
SERIALIZATION_ENUM(MovementStyle::Speed::Walk, "walk", "Walk")
SERIALIZATION_ENUM(MovementStyle::Speed::Run, "run", "Run")
SERIALIZATION_ENUM(MovementStyle::Speed::Sprint, "sprint", "Sprint")
SERIALIZATION_ENUM_END()
#endif // SWIG

//! \endcond INTERNAL
