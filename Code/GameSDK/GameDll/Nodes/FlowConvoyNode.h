// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Inplements a convoy in a FlowNode

-------------------------------------------------------------------------

*************************************************************************/

#ifndef __FLOWCONVOYNODE_H__
#define __FLOWCONVOYNODE_H__

#include <CryFlowGraph/IFlowSystem.h>
#include "Nodes/G2FlowBaseNode.h"

class CConvoyPathIterator
{
	friend class CConvoyPath;
public:
	CConvoyPathIterator();
	void Invalidate();

private:
	int Segment;
	float LastDistance;
};

class CConvoyPath
{
public:
	CConvoyPath();
	void SetPath(const std::vector<Vec3> &path);
	Vec3 GetPointAlongPath(float dist, CConvoyPathIterator &iterator);
	float GetTotalLength() const { return m_totalLength;}

private:
	struct SConvoyPathNode
	{
		Vec3 Position;
		float Length, TotalLength;
	};

	std::vector<SConvoyPathNode> m_path;
	float m_totalLength;
};

class CFlowConvoyNode : public CFlowBaseNode<eNCT_Instanced>
{
public:
	static std::vector<CFlowConvoyNode *> gFlowConvoyNodes;
	static int OnPhysicsPostStep_static(const EventPhys * pEvent);
	CFlowConvoyNode( SActivationInfo * pActInfo );
	~CFlowConvoyNode();
	virtual IFlowNodePtr Clone( SActivationInfo * pActInfo );
	virtual void Serialize(SActivationInfo *, TSerialize ser);
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
	virtual void GetMemoryUsage(ICrySizer * s) const;
	int OnPhysicsPostStep(const EventPhys * pEvent);
	static bool PlayerIsOnaConvoy();
private:
	void DiscoverConvoyCoaches(IEntity *pEntity);
	void InitConvoyCoaches();
	void AwakeCoaches();
	int GetCoachIndexPlayerIsOn();
	int GetCoachIndexPlayerIsOn2();
	float GetPlayerMaxVelGround();
	void SetPlayerMaxVelGround(float vel);

	void Update(SActivationInfo *pActInfo);

	//tSoundID PlayLineSound(int coachIndex, const char *sGroupAndSoundName, const Vec3 &vStart, const Vec3 &vEnd);
	void UpdateLineSounds();
	void StartSounds();
	void SplitLineSound();
	void ConvoyStopSounds();
	void StopAllSounds();
	float GetSpeedSoundParam(int coachIndex);
	void SetSoundParams();
	void StartBreakSoundShifted();

	enum EInputs
	{
		IN_PATH,
		IN_LOOPCOUNT,
		IN_SPEED,
		IN_DESIREDSPEED,
		IN_SHIFT,
		IN_SHIFTTIME,
		IN_START_DISTANCE,
		IN_SPLIT_COACH,
		IN_XAXIS_FWD,
		IN_HORN_SOUND,
		IN_BREAK_SOUND,
		IN_START,
		IN_STOP,
	};
	enum EOutputs
	{
		OUT_ONPATHEND,
		OUT_COACHINDEX,
	};

	CConvoyPath m_path;
	float m_speed;
	float m_desiredSpeed;
	float m_ShiftTime;
	float m_MaxShiftTime;
	float m_distanceOnPath; //train last coach end distance on path
	int m_splitCoachIndex;  //coach index where to split train (0 is the train engine)
	int m_loopCount;
	int m_loopTotal;

	bool m_bFirstUpdate;
	bool m_bXAxisFwd;
	CTimeValue m_offConvoyStartTime;
	int m_coachIndex;

	float m_splitDistanceOnPath; //the splitted coaches end coach distance on path
	float m_splitSpeed; //splitted coaches speed
	bool m_processNode;
	bool m_atEndOfPath;

	struct SConvoyCoach
	{
		SConvoyCoach();
		IEntity *m_pEntity;
		int m_frontWheelBase, m_backWheelBase;
		float m_wheelDistance; //wheel half distance from center
		float m_coachOffset;  //coach half length
		float m_distanceOnPath; //coach center distance on path
		IEntityAudioProxy* m_pEntitySoundsProxy;
		//tSoundID m_runSoundID;
		//tSoundID m_breakSoundID;
		CConvoyPathIterator m_frontWheelIterator[2], m_backWheelIterator[2];
	};

	//tSoundID m_hornSoundID;	
	//tSoundID m_engineStartSoundID;

	std::vector<SConvoyCoach> m_coaches;
	static float m_playerMaxVelGround;
	int m_startBreakSoundShifted;
};

#endif
