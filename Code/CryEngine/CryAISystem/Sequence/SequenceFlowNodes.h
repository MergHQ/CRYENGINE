// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SequenceManager.h"
#include <CryFlowGraph/IFlowBaseNode.h>

#include <CryAISystem/MovementRequestID.h>

struct MovementRequestResult;

namespace AIActionSequence
{

//////////////////////////////////////////////////////////////////////////

struct SequenceFlowNodeBase
	: public CFlowBaseNode<eNCT_Instanced>
	  , public SequenceActionBase
{
	// nothing
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceStart
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Interruptible,
		InputPort_ResumeAfterInterruption,
	};

	enum OutputPort
	{
		OutputPort_Link,
	};

	CFlowNode_AISequenceStart(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
		, m_restartOnPostSerialize(false)
		, m_waitingForTheForwardingEntityToBeSetOnAllTheNodes(false)
	{
	}

	virtual ~CFlowNode_AISequenceStart();

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceStart(pActInfo); }

	virtual void         Serialize(SActivationInfo* pActInfo, TSerialize ser) override;
	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         PostSerialize(SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

private:
	void InitializeSequence(SActivationInfo* pActInfo);

	bool            m_restartOnPostSerialize;
	bool            m_waitingForTheForwardingEntityToBeSetOnAllTheNodes;
	SActivationInfo m_actInfo;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceEnd
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_End,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceEnd(SActivationInfo* pActInfo) {}
	virtual ~CFlowNode_AISequenceEnd() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceEnd(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }

private:
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceBookmark
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_Set,
	};

	enum OutputPort
	{
		OutputPort_Link,
	};

	CFlowNode_AISequenceBookmark(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
	{}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceBookmark(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

private:
	SActivationInfo m_actInfo;
};

//////////////////////////////////////////////////////////////////////////

// Only used to inherit from.
class CFlowNode_AISequenceActionBase
	: public SequenceFlowNodeBase 
{
protected:
	CFlowNode_AISequenceActionBase(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
	{}

	// SequenceFlowNodeBase
	virtual void    ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void    HandleSequenceEvent(SequenceEvent sequenceEvent) override;
	// ~SequenceFlowNodeBase

private:

	enum InputPort
	{
		InputPort_Start,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

protected:

	SActivationInfo m_actInfo;
	bool            m_isRunning = false;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceActionMove
	: public CFlowNode_AISequenceActionBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Speed,
		InputPort_Stance,
		InputPort_DestinationEntity,
		InputPort_Position,
		InputPort_Direction,
		InputPort_EndDistance,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceActionMove(SActivationInfo* pActInfo)
		: CFlowNode_AISequenceActionBase(pActInfo)
		, m_stopRadiusSqr(0.0f)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceActionMove(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

	void                 MovementRequestCallback(const MovementRequestResult& result);

private:
	void GetPositionAndDirectionForDestination(OUT Vec3& position, OUT Vec3& direction);

	MovementRequestID m_movementRequestID;

	Vec3              m_destPosition;
	Vec3              m_destDirection;

	float             m_stopRadiusSqr;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceActionMoveAlongPath
	: public CFlowNode_AISequenceActionBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Speed,
		InputPort_Stance,
		InputPort_PathName,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceActionMoveAlongPath(SActivationInfo* pActInfo)
		: CFlowNode_AISequenceActionBase(pActInfo)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceActionMoveAlongPath(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

	void                 MovementRequestCallback(const MovementRequestResult& result);

private:
	void GetTeleportEndPositionAndDirection(const char* pathName, Vec3& position, Vec3& direction);

	MovementRequestID m_movementRequestID;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceActionAnimation
	: public CFlowNode_AISequenceActionBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Stop,
		InputPort_Animation,
		InputPort_DestinationEntity,
		InputPort_Position,
		InputPort_Direction,
		InputPort_Speed,
		InputPort_Stance,
		InputPort_OneShot,
		InputPort_StartRadius,
		InputPort_DirectionTolerance,
		InputPort_LoopDuration,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceActionAnimation(SActivationInfo* pActInfo)
		: CFlowNode_AISequenceActionBase(pActInfo)
		, m_bTeleportWhenNotMoving(false)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceActionAnimation(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

	void                 MovementRequestCallback(const MovementRequestResult& result);

private:
	void       GetPositionAndDirectionForDestination(Vec3& position, Vec3& direction);
	void       ClearAnimation(bool bHurry);
	IAIObject* GetAssignedEntityAIObject();

	MovementRequestID m_movementRequestID;

	Vec3              m_destPosition;
	Vec3              m_destDirection;
	bool              m_bTeleportWhenNotMoving;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceActionWait
	: public CFlowNode_AISequenceActionBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Time,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceActionWait(SActivationInfo* pActInfo)
		: CFlowNode_AISequenceActionBase(pActInfo)
		, m_waitTimeMs(0)
	{}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceActionWait(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override ;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }

	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;
private:
	CTimeValue m_startTime;
	int m_waitTimeMs;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceActionShoot
	: public CFlowNode_AISequenceActionBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_TargetEntity,
		InputPort_TargetPosition,
		InputPort_Duration,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceActionShoot(SActivationInfo* pActInfo)
		: CFlowNode_AISequenceActionBase(pActInfo)
		, m_fireTimeMS(0)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceActionShoot(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

private:

	CTimeValue m_startTime;
	int m_fireTimeMS;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceHoldFormation
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_FormationName,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceHoldFormation(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
	{
	}

	virtual ~CFlowNode_AISequenceHoldFormation() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceHoldFormation(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }

private:
	SActivationInfo m_actInfo;
};

class CFlowNode_AISequenceJoinFormation
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_LeaderId,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceJoinFormation(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
	{
	}

	virtual ~CFlowNode_AISequenceJoinFormation() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceJoinFormation(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }

private:

	void SendSignal(IAIActor* pIAIActor, const char* signalName, IEntity* pSender);

	SActivationInfo m_actInfo;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_AISequenceAction_Stance
	: public SequenceFlowNodeBase
{
public:
	enum InputPort
	{
		InputPort_Start,
		InputPort_Stance,
	};

	enum OutputPort
	{
		OutputPort_Done,
	};

	CFlowNode_AISequenceAction_Stance(SActivationInfo* pActInfo)
		: m_actInfo(*pActInfo)
	{
	}

	virtual ~CFlowNode_AISequenceAction_Stance() {}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override { return new CFlowNode_AISequenceAction_Stance(pActInfo); }

	virtual void         GetConfiguration(SFlowNodeConfig& config) override;
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override;
	virtual void         GetMemoryUsage(ICrySizer* sizer) const override { sizer->Add(*this); }
	virtual void         HandleSequenceEvent(SequenceEvent sequenceEvent) override;

private:
	SActivationInfo m_actInfo;
};

//////////////////////////////////////////////////////////////////////////

} // namespace AIActionSequence
