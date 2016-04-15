// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowConvoyNode.h"
#include <CryAISystem/INavigation.h>
#include "Player.h"
#include "Item.h"
#include "GameCVars.h"

#define MAX_TRAIN_SPEED 8.f  //for sound
#define COACH_NUMBER_SOUND_TOGETHER 2


float CFlowConvoyNode::m_playerMaxVelGround=0;

CConvoyPathIterator::CConvoyPathIterator()
{
	Invalidate();
}

void CConvoyPathIterator::Invalidate()
{
	Segment = 0;
	LastDistance = 0;
}

CConvoyPath::CConvoyPath()
: m_totalLength(0.0f)
{
}

void CConvoyPath::SetPath(const std::vector<Vec3> &path)
{
	int num_path_nodes = path.size();
	m_path.resize(num_path_nodes);
	m_totalLength = 0.0f;
	for (int i=0; i<num_path_nodes; i++)
	{
		m_path[i].Position = path[i];
		if (i<num_path_nodes-1)
			m_path[i].Length = path[i].GetDistance(path[i+1]);
		else
			m_path[i].Length = 0;

		m_path[i].TotalLength = m_totalLength;
		m_totalLength += m_path[i].Length;
	}
}

Vec3 CConvoyPath::GetPointAlongPath(float dist, CConvoyPathIterator &iterator)
{
	int size = m_path.size();

	if (!size)
		return Vec3(0, 0, 0);
	if (dist <= 0)
		return m_path[0].Position;
	if (dist < iterator.LastDistance)
		iterator.Invalidate();

	while (iterator.Segment+1 < size && dist > m_path[iterator.Segment+1].TotalLength)
		iterator.Segment++;

	iterator.LastDistance = dist;

	if (iterator.Segment+1 == size)
		return m_path[iterator.Segment].Position;

	return Vec3::CreateLerp(m_path[iterator.Segment].Position, m_path[iterator.Segment+1].Position, (dist - m_path[iterator.Segment].TotalLength)/m_path[iterator.Segment].Length);
}

CFlowConvoyNode::SConvoyCoach::SConvoyCoach()
{
	m_pEntity=NULL;
	m_frontWheelBase=m_backWheelBase=-1;
	m_wheelDistance=0.f;
	m_coachOffset=0.f;
	m_distanceOnPath=0.f;
	m_pEntitySoundsProxy=NULL;
	//m_runSoundID=INVALID_SOUNDID;
	//m_breakSoundID=INVALID_SOUNDID;
}


int CFlowConvoyNode::OnPhysicsPostStep_static(const EventPhys * pEvent)
{
	for (std::vector<CFlowConvoyNode *>::iterator it = gFlowConvoyNodes.begin(); gFlowConvoyNodes.end() != it; ++it)
	{
		CFlowConvoyNode *node = *it;

		if (node->m_processNode)
		{
			node->OnPhysicsPostStep(pEvent);
		}
	}

	return 0;
}

CFlowConvoyNode::CFlowConvoyNode( SActivationInfo * pActInfo )
{
	m_distanceOnPath = 0;
	m_bFirstUpdate = true;
	m_bXAxisFwd = false;
	m_speed = 0.0f;
	m_desiredSpeed = 0.0f;
	m_ShiftTime = 0.0f;
	m_splitSpeed = 0;
	m_coachIndex = -1;
	m_splitCoachIndex = 0;
	m_loopCount = 0;
	m_loopTotal = 0;
	m_offConvoyStartTime=0.f;
	m_splitDistanceOnPath=0;
	//m_hornSoundID=INVALID_SOUNDID;
	//m_engineStartSoundID=INVALID_SOUNDID;
	m_startBreakSoundShifted=0;
	pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
	if (gFlowConvoyNodes.empty())
		gEnv->pPhysicalWorld->AddEventClient(EventPhysPostStep::id, OnPhysicsPostStep_static, 0);
	gFlowConvoyNodes.push_back(this);

	ICVar* pICVar = gEnv->pConsole->GetCVar("es_UsePhysVisibilityChecks");
	if(pICVar) //prevent distance problem between coaches
		pICVar->Set(0);

	m_processNode = false;
	m_atEndOfPath = false;
}

CFlowConvoyNode::~CFlowConvoyNode()
{
	for (size_t i=0; i<gFlowConvoyNodes.size(); ++i)
	{
		if (gFlowConvoyNodes[i] == this)
		{
			gFlowConvoyNodes.erase(gFlowConvoyNodes.begin() + i);
			break;
		}
	}
	if (gFlowConvoyNodes.empty())
	{
		gEnv->pPhysicalWorld->RemoveEventClient(EventPhysPostStep::id, OnPhysicsPostStep_static, 0);
		stl::free_container(gFlowConvoyNodes);
	}

	ICVar* pICVar = gEnv->pConsole->GetCVar("es_UsePhysVisibilityChecks");
	if(pICVar)
		pICVar->Set(1);
}

IFlowNodePtr CFlowConvoyNode::Clone( SActivationInfo * pActInfo )
{
	return new CFlowConvoyNode(pActInfo);
}

void CFlowConvoyNode::Serialize(SActivationInfo *, TSerialize ser)
{
	ser.BeginGroup("Local");
	ser.Value("m_distanceOnPath", m_distanceOnPath);
	ser.Value("m_speed", m_speed);
	ser.Value("m_desiredSpeed", m_desiredSpeed);
	ser.Value("m_offConvoyStartTime", m_offConvoyStartTime);
	ser.Value("m_coachIndex", m_coachIndex);
	ser.Value("m_loopCount", m_loopCount);
	ser.Value("m_loopTotal", m_loopTotal);
	ser.Value("m_splitCoachIndex", m_splitCoachIndex);
	ser.Value("m_splitSpeed", m_splitSpeed);
	ser.Value("m_splitDistanceOnPath", m_splitDistanceOnPath);
	ser.Value("m_startBreakSoundShifted", m_startBreakSoundShifted);
	ser.Value("m_bXAxisFwd", m_bXAxisFwd);
	ser.Value("m_atEndOfPath", m_atEndOfPath);
	ser.EndGroup();

	if(ser.IsReading()) //after load must do some initializing in first update
		m_bFirstUpdate = true;
}

void CFlowConvoyNode::GetConfiguration( SFlowNodeConfig &config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig<string>( "Path",_HELP("Path to move the train on") ),
		InputPortConfig<int>( "LoopCount",0,_HELP("How many times to loop along the path (-1 for infinite)") ),
		InputPortConfig<float>( "Speed",_HELP("Speed in m/s (not work with negative speed)") ),
		InputPortConfig<float>( "DesiredSpeed",_HELP("Speed in m/s (not work with negative speed)") ),
		InputPortConfig<bool>( "Shift",_HELP("dO SHIFT") ),
		InputPortConfig<float>( "ShiftTime",_HELP("Speed in m/s (not work with negative speed)") ),
		InputPortConfig<float>( "StartDistance",_HELP("Start distance of the last coach end (from the start of the path)") ),
		InputPortConfig<int>( "SplitCoachNumber",_HELP("If acticated, the train splits from this coach") ),
		InputPortConfig<bool>( "XAxisIsForward", false, _HELP("If true, and coaches are not CHRs, will make X axis forward instead of Y") ),
		InputPortConfig_Void( "PlayHornSound",_HELP("If acticated, train horn sound played") ),
		InputPortConfig_Void( "PlayBreakSound",_HELP("If acticated, train break sound played") ),
		InputPortConfig_Void( "Start",_HELP("Trigger to start the convoy") ),
		InputPortConfig_Void( "Stop",_HELP("Trigger to stop the convoy") ),
		{0}
	};
	static const SOutputPortConfig out_config[] = {
		OutputPortConfig_Void("onPathEnd", _HELP("Triggers when first coach reaches the end of the path")),
		OutputPortConfig<int> ("PlayerCoachIndex", _HELP("gives the coach index Player is standing on, (-1) if not on train for at least 4 seconds")),
		{0}
	};

	config.sDescription = _HELP( "Convoy node will move a special entity on an AIPath with speed" );
	config.pInputPorts = in_config;
	config.pOutputPorts = out_config;
	config.SetCategory(EFLN_APPROVED);
	config.nFlags |= EFLN_TARGET_ENTITY;
}

#define PATH_DERIVATION_TIME 0.1f

static float breakValue = 0.5f;

int CFlowConvoyNode::OnPhysicsPostStep(const EventPhys * pEvent)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

  if(m_bFirstUpdate)
    return 0; //after QuickLoad OnPhysicsPostStep called before ProcessEvent

	const EventPhysPostStep *pPhysPostStepEvent = (const EventPhysPostStep *)pEvent;

	IPhysicalEntity *pEventPhysEnt = pPhysPostStepEvent->pEntity;

	Vec3 posBack, posFront, posCenter;

	for (size_t i = 0; i < m_coaches.size(); ++i)
	{
		IPhysicalEntity *pPhysEnt = m_coaches[i].m_pEntity->GetPhysics();

		if (pEventPhysEnt == pPhysEnt)
		{
			if (m_ShiftTime > 0.0f)
			{
				m_speed = m_speed * (1.0f - breakValue * pPhysPostStepEvent->dt / m_MaxShiftTime);
				m_ShiftTime -= pPhysPostStepEvent->dt;
			}
			else
			{
				m_speed = m_speed + min(1.0f, (pPhysPostStepEvent->dt / 5.0f)) * (m_desiredSpeed - m_speed);
			}
		

			float speed = (m_splitCoachIndex > 0 && (int)i >= m_splitCoachIndex) ? m_splitSpeed : m_speed;
			float distance = (m_coaches[i].m_distanceOnPath += pPhysPostStepEvent->dt * speed);
			
			if(m_splitCoachIndex>0)
			{//train splitted
				if(i==m_splitCoachIndex-1) // update m_distanceOnPath for serialization
					m_distanceOnPath=distance-m_coaches[i].m_coachOffset;
				else if(i==m_coaches.size()-1) // update m_splitDistanceOnPath for serialization
					m_splitDistanceOnPath=distance-m_coaches[i].m_coachOffset;
			}
			else
			{//train in one piece
				if(i==m_coaches.size()-1)// update m_distanceOnPath for serialization
					m_distanceOnPath=distance-m_coaches[i].m_coachOffset;
			}

			posBack  = m_path.GetPointAlongPath(distance - m_coaches[i].m_wheelDistance, m_coaches[i].m_backWheelIterator[0]);
			posFront = m_path.GetPointAlongPath(distance + m_coaches[i].m_wheelDistance, m_coaches[i].m_frontWheelIterator[0]);
			posCenter = (posBack+posFront)*0.5f;


			Vec3 posDiff = posFront - posBack;
			float magnitude = posDiff.GetLength();

			if (magnitude > FLT_EPSILON)
			{
				posDiff *= 1.0f / magnitude;

				pe_params_pos ppos;
				ppos.pos = posCenter;
				ppos.q = Quat::CreateRotationVDir(posDiff);
				if (m_bXAxisFwd)
					ppos.q *= Quat::CreateRotationZ(gf_PI*-0.5f);
				pPhysEnt->SetParams(&ppos, 0 /* bThreadSafe=0 */); // as we are calling from the physics thread

				Vec3 futurePositionBack, futurePositionFront;
				futurePositionBack  = m_path.GetPointAlongPath(distance + PATH_DERIVATION_TIME * speed - m_coaches[i].m_wheelDistance, m_coaches[i].m_backWheelIterator[1]);
				futurePositionFront = m_path.GetPointAlongPath(distance + PATH_DERIVATION_TIME * speed + m_coaches[i].m_wheelDistance, m_coaches[i].m_frontWheelIterator[1]);

				Vec3 futurePosDiff = futurePositionFront - futurePositionBack;
				magnitude = futurePosDiff.GetLength();

				if (magnitude > FLT_EPSILON)
				{
					futurePosDiff *= 1.0f / magnitude;

					pe_action_set_velocity setVel;
					setVel.v = ((futurePositionBack+ futurePositionFront)*0.5f - posCenter) * (1.0f/PATH_DERIVATION_TIME);

					//Vec3 dir=(posFront-posBack).GetNormalized();
					//Vec3 future_dir=(futurePositionFront-futurePositionBack).GetNormalized();
					Vec3 w=posDiff.Cross(futurePosDiff);
					float angle=asin_tpl(w.GetLength());
					setVel.w=(angle/PATH_DERIVATION_TIME)*w.GetNormalized();
					pPhysEnt->Action(&setVel, 0 /* bThreadSafe=0 */); // as we are calling from the physics thread
					break;
				}
			}
		}
	}

	// Done processing once we reach end of path
	if (m_atEndOfPath)
		m_processNode = false;

	return 0;
}

bool CFlowConvoyNode::PlayerIsOnaConvoy()
{
	for(size_t i = 0; i < gFlowConvoyNodes.size(); ++i)
	{
		if(gFlowConvoyNodes[i]->m_coachIndex>=0)
			return true;
	}
	return false;
}

void CFlowConvoyNode::DiscoverConvoyCoaches(IEntity *pEntity)
{
	m_coaches.resize(0);

	while (pEntity)
	{
		SConvoyCoach tc;

		ICharacterInstance *pCharacterInstance = pEntity->GetCharacter(0);
		ISkeletonPose *pSkeletonPose = pCharacterInstance ? pCharacterInstance->GetISkeletonPose() : NULL;
		IDefaultSkeleton* pIDefaultSkeleton = pCharacterInstance ? &pCharacterInstance->GetIDefaultSkeleton() : NULL;
		IPhysicalEntity* pPhysics = pEntity->GetPhysics();
		if(!pPhysics) 
		{
			// don't need physics here, but need it later, so don't use entity if it's not physicalized
			GameWarning("Convoy entity [%s] is not physicalized", pEntity->GetName());
			break;
		}

		AABB bbox;
		pEntity->GetLocalBounds(bbox);

		//tc.m_coachOffset = (bbox.max.y - bbox.min.y) * .5f;
		tc.m_coachOffset = 10.0f;

		tc.m_frontWheelBase = pIDefaultSkeleton ? pIDefaultSkeleton->GetJointIDByName("wheel_base1") : -1;
		tc.m_backWheelBase = pIDefaultSkeleton ? pIDefaultSkeleton->GetJointIDByName("wheel_base2") : -1;
		
		if (tc.m_frontWheelBase >=0 && tc.m_backWheelBase >= 0)
		{
			QuatT qt1 = pSkeletonPose->GetRelJointByID(tc.m_frontWheelBase);
			QuatT qt2 = pSkeletonPose->GetRelJointByID(tc.m_backWheelBase);
			tc.m_wheelDistance = qt1.t.GetDistance(qt2.t) * .5f;
		}
		else
		{
			// Fallback for entities that don't have wheel_base joints
			if ( m_bXAxisFwd )
				tc.m_wheelDistance = (bbox.max.x - bbox.min.x) * .5f;
			else
				tc.m_wheelDistance = (bbox.max.y - bbox.min.y) * .5f;
		}

		tc.m_pEntity = pEntity;
  //  pEntity->SetConvoyEntity();
		//for (int i = 0; i < pEntity->GetChildCount(); i++)
		//	pEntity->GetChild(i)->SetConvoyEntity();

		//tc.m_pEntitySoundsProxy = (IEntityAudioProxy*) tc.m_pEntity->CreateProxy(ENTITY_PROXY_AUDIO);
		//assert(tc.m_pEntitySoundsProxy);

		m_coaches.push_back(tc);
		IEntityLink *pEntityLink = pEntity->GetEntityLinks();
		pEntity = pEntityLink ? gEnv->pEntitySystem->GetEntity(pEntityLink->entityId) : NULL;
	}
}


void CFlowConvoyNode::InitConvoyCoaches()
{
	float distance = m_splitCoachIndex>0 ? m_splitDistanceOnPath : m_distanceOnPath;

	for (int i=m_coaches.size()-1; i>=0; --i)
	{
		if(m_splitCoachIndex>0 && i==m_splitCoachIndex-1)
			distance=m_distanceOnPath; // find the first train part end distance
		
		distance += m_coaches[i].m_coachOffset;
		m_coaches[i].m_distanceOnPath=distance;			
		distance += m_coaches[i].m_coachOffset;

		IPhysicalEntity* pPhysics = m_coaches[i].m_pEntity->GetPhysics();
		pe_params_flags flags;
		flags.flagsOR = pef_monitor_poststep|pef_fixed_damping;
		pPhysics->SetParams(&flags);

		pe_params_foreign_data pfd;
		pfd.iForeignFlagsOR = 0; //PFF_ALWAYS_VISIBLE;
		pPhysics->SetParams(&pfd);

		pe_simulation_params sp;
		sp.mass = -1;
		sp.damping = sp.dampingFreefall = 0;
		pPhysics->SetParams(&sp);
	}
}

void CFlowConvoyNode::AwakeCoaches() //awake the physics
{
	pe_action_awake aa; 
	aa.bAwake=1;
	for (size_t i = 0; i < m_coaches.size(); ++i)
	{
		IPhysicalEntity* pPhysics = m_coaches[i].m_pEntity->GetPhysics();
		pPhysics->Action(&aa);
	}
}

int CFlowConvoyNode::GetCoachIndexPlayerIsOn()
{
	IPhysicalEntity *pGroundCollider=NULL;
	IActor *pPlayerActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

	// if player use a mounted weapon on the train, the GroundCollider check not good
	if(m_coachIndex>=0)
	{//player can catch mounted weapon on the train if he were on it before
		CItem *pCurrentItem=static_cast<CItem *>(pPlayerActor->GetCurrentItem());
		if ( pCurrentItem != NULL && pCurrentItem->IsMounted()) 
			return m_coachIndex; // give back the last m_coachIndex, it should be valid 		
	}

	IPhysicalEntity *pPhysicalEntity=pPlayerActor->GetEntity()->GetPhysics();
	if (pPhysicalEntity)
	{
		pe_status_living livStat;
		if(pPhysicalEntity->GetStatus(&livStat))
			pGroundCollider=livStat.pGroundCollider;
	}
	
	if(!pGroundCollider)
		return -1;

	for (size_t i = 0; i < m_coaches.size(); ++i)
	{
		if(m_coaches[i].m_pEntity->GetPhysics()==pGroundCollider)
			return i;
		else
		{//attached objects
			IEntity *pCoachEntity=m_coaches[i].m_pEntity;
			for (int j = 0; j < pCoachEntity->GetChildCount(); ++j)
			{
				if(pCoachEntity->GetChild(j)->GetPhysics()==pGroundCollider)
					return i;
			}
		}
	}
	return -1;
}

int CFlowConvoyNode::GetCoachIndexPlayerIsOn2()
{
	IActor *pPlayerActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	Vec3 playerPos=pPlayerActor->GetEntity()->GetWorldPos();
	for (size_t i = 0; i < m_coaches.size(); ++i)
	{
		AABB bbox;
		m_coaches[i].m_pEntity->GetLocalBounds(bbox);
		Vec3 localpos=m_coaches[i].m_pEntity->GetWorldTM().GetInverted().TransformPoint(playerPos);
		if(bbox.min.x<=localpos.x && bbox.min.y<=localpos.y && bbox.max.x>=localpos.x && bbox.max.y>=localpos.y)
			return i;
	}
	return -1;
}

float CFlowConvoyNode::GetPlayerMaxVelGround()
{
	IActor *pPlayerActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pPlayerActor)
	{
		IPhysicalEntity *pPhysicalEntity=pPlayerActor->GetEntity()->GetPhysics();
		if(pPhysicalEntity)
		{
			pe_player_dynamics ppd;
			pPhysicalEntity->GetParams(&ppd);
			return ppd.maxVelGround;
		}
	}
	return 0;
}

void CFlowConvoyNode::SetPlayerMaxVelGround(float vel)
{
	IActor *pPlayerActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pPlayerActor)
	{
		IPhysicalEntity *pPhysicalEntity=pPlayerActor->GetEntity()->GetPhysics();
		if(pPhysicalEntity)
		{
			pe_player_dynamics ppd;
			ppd.maxVelGround=vel;
			pPhysicalEntity->SetParams(&ppd);
		}
	}
}

#define SHIFT_FRAME 5

void CFlowConvoyNode::StartBreakSoundShifted()
{
	//if(m_startBreakSoundShifted%SHIFT_FRAME==1)
	//{
	//	bool isPlaying=false;
	//	int index=m_startBreakSoundShifted/SHIFT_FRAME;
	//	if(m_coaches[index].m_breakSoundID!=INVALID_SOUNDID)
	//	{
	//		ISound *i_sound=m_coaches[index].m_pEntitySoundsProxy->GetSound(m_coaches[index].m_breakSoundID);
	//		if(i_sound && i_sound->IsPlaying())
	//			isPlaying=true;
	//	}
	//	if(!isPlaying)
	//		m_coaches[index].m_breakSoundID=m_coaches[index].m_pEntitySoundsProxy->PlaySound("sounds/vehicles_exp1:train:breaking", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, FLAG_SOUND_DEFAULT_3D, eSoundSemantic_Vehicle);
	//	if(index==m_coaches.size()-1)
	//		m_startBreakSoundShifted=-1;
	//}
	//m_startBreakSoundShifted++;
}

#define BIG_MAX_VEL_GROUND 1000000.f
#define SPLIT_ACCEL -0.5f  //splitted coaches acceleration
#define OFF_TRAIN_TIME 4.f

void CFlowConvoyNode::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	switch (event)
	{
	case eFE_Update:
		{
			if (m_processNode)
				Update(pActInfo);
		}
		break;
	case eFE_Activate:
		{
			if (IsPortActive(pActInfo, IN_SPEED) || IsPortActive(pActInfo, IN_DESIREDSPEED))
			{
				float speed=0;

				speed=GetPortFloat(pActInfo, IN_DESIREDSPEED);

				if(speed<0) //do not use negative speed
					speed=0;
				if(speed>m_speed && m_speed<0.01f)
				{
					AwakeCoaches(); //if speed is too small Physics go to sleep and callback does not work...
													//have to awake if train start again. should make more elegant...(keep awake somehow)
					StartSounds();
				}

				if(speed==0.f && m_speed>0.f)
					ConvoyStopSounds();

				m_desiredSpeed = speed;
			}
			else if(IsPortActive(pActInfo, IN_SPLIT_COACH))
			{
				if(!m_splitCoachIndex)
				{
					int splitCoachIndex=GetPortInt(pActInfo, IN_SPLIT_COACH);
					if(splitCoachIndex > 0 && splitCoachIndex < (int)m_coaches.size())
					{
						m_splitCoachIndex=splitCoachIndex;
						m_splitDistanceOnPath=m_distanceOnPath;
						m_distanceOnPath=m_coaches[m_splitCoachIndex-1].m_distanceOnPath-m_coaches[m_splitCoachIndex-1].m_coachOffset;
						m_splitSpeed=m_speed;
						if(!(m_splitCoachIndex&1))
							SplitLineSound(); //have to split the lineSound (there is 1 sound for 2 coaches)
					}
				}
				else
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "SplitCoachIndex has been set! Do not set it again! FlowConvoyNode: %s", m_coaches[0].m_pEntity->GetName());
			}
			else if(IsPortActive(pActInfo, IN_HORN_SOUND))
			{
				//bool isPlaying=false;
				//if(m_hornSoundID!=INVALID_SOUNDID)
				//{
				//	ISound *i_sound=m_coaches[0].m_pEntitySoundsProxy->GetSound(m_hornSoundID);
				//	if(i_sound && i_sound->IsPlaying())
				//		isPlaying=true;
				//}
				//if(!isPlaying)
				//	m_hornSoundID=m_coaches[0].m_pEntitySoundsProxy->PlaySound("sounds/vehicles_exp1:train:horn", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, FLAG_SOUND_DEFAULT_3D, eSoundSemantic_Vehicle);
			}
			else if(IsPortActive(pActInfo, IN_BREAK_SOUND) && m_speed>0.f)
				m_startBreakSoundShifted=1;
			else if(IsPortActive(pActInfo, IN_START_DISTANCE))
			{
				m_distanceOnPath = GetPortFloat(pActInfo, IN_START_DISTANCE);
				InitConvoyCoaches();
			}
			
			if (IsPortActive(pActInfo, IN_SHIFT))
			{
				m_ShiftTime = m_MaxShiftTime = GetPortFloat(pActInfo, IN_SHIFTTIME);
				bool* p_x = (pActInfo->pInputPorts[IN_SHIFT].GetPtr<bool>());

				*p_x = false;
			}
		}
		
		if (IsPortActive(pActInfo, IN_START))
		{
			m_processNode = true;
		}
		else if (IsPortActive(pActInfo, IN_STOP))
		{
			m_processNode = false;
		}
		break;		
	case eFE_Initialize:
		m_distanceOnPath = GetPortFloat(pActInfo, IN_START_DISTANCE);
		m_speed					= GetPortFloat(pActInfo, IN_SPEED);
		m_desiredSpeed	= GetPortFloat(pActInfo, IN_DESIREDSPEED);


		std::vector<Vec3> temp;
		temp.resize(1000);
		uint32 count = gEnv->pAISystem->GetINavigation()->GetPath(GetPortString(pActInfo, IN_PATH), &temp[0], 1000);
		temp.resize(count);

		m_path.SetPath(temp);
		m_splitCoachIndex = GetPortInt(pActInfo,IN_SPLIT_COACH);
		m_loopCount = 0;
		m_loopTotal = GetPortInt(pActInfo,IN_LOOPCOUNT);
		m_bXAxisFwd = GetPortBool(pActInfo, IN_XAXIS_FWD);
		m_bFirstUpdate = true;
		m_atEndOfPath = false;
		StopAllSounds();
		break;
	}
}

void CFlowConvoyNode::Update(SActivationInfo *pActInfo)
{
	assert(pActInfo);

	if (m_bFirstUpdate)
	{
		DiscoverConvoyCoaches(pActInfo->pEntity);
		InitConvoyCoaches();
		AwakeCoaches();
		if(m_speed>0.f)
			StartSounds();
		if(!m_playerMaxVelGround)
			m_playerMaxVelGround=GetPlayerMaxVelGround();
		if(m_coachIndex<0)
			SetPlayerMaxVelGround(m_playerMaxVelGround);
		else
			SetPlayerMaxVelGround(BIG_MAX_VEL_GROUND);
		m_bFirstUpdate = false;
	}

	//int coachIndex=GetCoachIndexPlayerIsOn();
	int coachIndex=GetCoachIndexPlayerIsOn2();

	if(coachIndex==-1 && m_coachIndex!=-1) 
	{//before on the train but get off
		/*if(m_offConvoyStartTime==0.f)
		m_offConvoyStartTime = gEnv->pTimer->GetFrameStartTime();
		else
		if((gEnv->pTimer->GetFrameStartTime()-m_offConvoyStartTime).GetSeconds()>OFF_TRAIN_TIME)*/
		{
			m_coachIndex=-1;
			m_offConvoyStartTime=0.f;
			ActivateOutput(pActInfo, OUT_COACHINDEX, m_coachIndex);
			SetPlayerMaxVelGround(m_playerMaxVelGround);
			return;
		}
	}
	else if(coachIndex!=m_coachIndex)
	{//changed coach or get on another
		m_coachIndex=coachIndex;
		ActivateOutput(pActInfo, OUT_COACHINDEX, m_coachIndex);
		SetPlayerMaxVelGround(BIG_MAX_VEL_GROUND);
	}
	else if(m_offConvoyStartTime!=0.f && coachIndex>-1)
		m_offConvoyStartTime=0.f;// was off train but get on the same coach within time

	if(m_splitCoachIndex>0 && m_splitSpeed>0)
	{// slow down splitted coaches automatically
		m_splitSpeed+=SPLIT_ACCEL*gEnv->pTimer->GetFrameTime();
		if(m_splitSpeed<0)
			m_splitSpeed=0;
	}

	if(m_speed || m_splitSpeed)
	{
		SetSoundParams();
		UpdateLineSounds();
		if(m_startBreakSoundShifted)
			StartBreakSoundShifted();
	}

	// Stop if first coach reaches path end
	if (!m_coaches.empty() && m_coaches[0].m_distanceOnPath >= m_path.GetTotalLength())
	{
		// Check for looping case
		if (++m_loopCount <= m_loopTotal || m_loopTotal < 0)
		{
			m_distanceOnPath = 0.0f;
			InitConvoyCoaches();
		}
		else
		{
			m_speed = 0.0f;
			m_atEndOfPath = true;
		}

		ActivateOutput(pActInfo, OUT_ONPATHEND, true);
	}
}

 float CFlowConvoyNode::GetSpeedSoundParam(int coachIndex)
{
	float speedParam;
	if(m_splitCoachIndex>0 && coachIndex>=m_splitCoachIndex)
		speedParam =m_splitSpeed;
	else
		speedParam = m_speed;

	speedParam=speedParam/MAX_TRAIN_SPEED;
	return speedParam>1.f ? 1.f : speedParam;
}

void CFlowConvoyNode::SetSoundParams()
{
	//for (int i=0; i<m_coaches.size(); ++i)
	//{
	//	if(m_coaches[i].m_runSoundID!=INVALID_SOUNDID && m_coaches[i].m_pEntitySoundsProxy)
	//	{
	//		ISound *i_sound=m_coaches[i].m_pEntitySoundsProxy->GetSound(m_coaches[i].m_runSoundID);
	//		if(i_sound)
	//			i_sound->SetParam("speed",GetSpeedSoundParam(i), false);
	//		//else
	//			//CryLogAlways("No ISound:%d", i);
	//	}
	//}
}

//tSoundID CFlowConvoyNode::PlayLineSound(int coachIndex, const char *sGroupAndSoundName, const Vec3 &vStart, const Vec3 &vEnd)
//{
//	//IEntityAudioProxy* pIEntityAudioProxy = m_coaches[coachIndex].m_pEntitySoundsProxy;
//	//EntityId SkipEntIDs[1];
//	//SkipEntIDs[0] = m_coaches[coachIndex].m_pEntity->GetId();
//
//	tSoundID ID = INVALID_SOUNDID;
//
//	//if (!gEnv->pSoundSystem)
//	//	return ID;
//
//	//ISound *pSound = gEnv->pSoundSystem->CreateLineSound(sGroupAndSoundName, FLAG_SOUND_DEFAULT_3D, vStart, vEnd);
//
//	//if (pSound)
//	//{
//	//	pSound->SetSemantic(eSoundSemantic_Vehicle);
//	//	pSound->SetPhysicsToBeSkipObstruction(SkipEntIDs, 1);
//
//	//	if (pIEntityAudioProxy->PlaySound(pSound, Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY))
//	//		ID = pSound->GetId();
//	//}
//	
//	return ID;
//}

void CFlowConvoyNode::UpdateLineSounds()
{
	//for (int i=0; i<m_coaches.size(); ++i)
	//{
	//	if(m_coaches[i].m_runSoundID!=INVALID_SOUNDID && m_coaches[i].m_pEntitySoundsProxy)
	//	{
	//		ISound *i_sound=m_coaches[i].m_pEntitySoundsProxy->GetSound(m_coaches[i].m_runSoundID);
	//		if(i_sound)
	//		{				
	//			bool splitSound1=m_splitCoachIndex && !(m_splitCoachIndex&1) && (m_splitCoachIndex==i || m_splitCoachIndex==i+1);
	//			bool splitSound2=(splitSound1 && i==m_splitCoachIndex && m_splitCoachIndex+1==m_coaches.size()-1);
	//			Vec3 lineStart, lineEnd;
	//			if(!i || (splitSound1 && !splitSound2) ) 
	//			{//line sound for train engine or one split coach
	//				Vec3 pos=m_coaches[i].m_pEntity->GetWorldPos();
	//				Vec3 forwardDir=m_coaches[i].m_pEntity->GetWorldTM().GetColumn1();
	//				lineStart=pos-forwardDir*m_coaches[i].m_coachOffset;
	//				lineEnd=pos+forwardDir*m_coaches[i].m_coachOffset;
	//			}
	//			else 
	//			{
	//				Vec3 pos2;
	//				float halfLineLength;
	//				Vec3 pos1=m_coaches[i].m_pEntity->GetWorldPos();
	//				if(i+2==m_coaches.size()-1)
	//				{//line sound for two coaches and the last coach (3 together)
	//					pos2=m_coaches[i+2].m_pEntity->GetWorldPos();
	//					halfLineLength=m_coaches[i].m_coachOffset+m_coaches[i+1].m_coachOffset+m_coaches[i+2].m_coachOffset;
	//				}
	//				else
	//				{//line sound for two coaches
	//					pos2=m_coaches[i+1].m_pEntity->GetWorldPos();
	//					halfLineLength=m_coaches[i].m_coachOffset+m_coaches[i+1].m_coachOffset;
	//				}
	//				Vec3 forwardDir=(pos1-pos2).GetNormalizedSafe();

	//				lineStart=(pos1+pos2)*0.5f+forwardDir*halfLineLength;
	//				lineEnd=(pos1+pos2)*0.5f-forwardDir*halfLineLength;
	//			}

	//			i_sound->SetLineSpec(lineStart, lineEnd);
	//			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lineStart+Vec3(0,0,2.f), ColorB(0,255,0,255), lineEnd+Vec3(0,0,2.f), ColorB(0,255,0,255), 2.0f);
	//		}
	//		//else
	//		//CryLogAlways("No ISound:%d", i);
	//	}
	//}
}

void CFlowConvoyNode::StartSounds()
{

	//for (int i=0; i<m_coaches.size(); ++i)
	//{
	//	if(m_coaches[i].m_pEntitySoundsProxy)
	//	{
	//		if(m_coaches[i].m_runSoundID != INVALID_SOUNDID)
	//			m_coaches[i].m_pEntitySoundsProxy->StopAllSounds();
	//		
	//		if(!i)
	//		{
	//			m_engineStartSoundID=m_coaches[0].m_pEntitySoundsProxy->PlaySound("sounds/vehicles_exp1:train:engine_start", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, FLAG_SOUND_DEFAULT_3D, eSoundSemantic_Vehicle);
	//			m_coaches[0].m_runSoundID=m_engineStartSoundID=PlayLineSound(0, "sounds/vehicles_exp1:train:engine", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_Zero);
	//		}//the LineSound line start and end parameter will be filled later in UpdateLineSound()
	//		else if(i&1 && i!=m_coaches.size()-1) //set sound for every second coach to prevent too much sound
	//			m_coaches[i].m_runSoundID=m_engineStartSoundID=PlayLineSound(i, "sounds/vehicles_exp1:train:run", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_Zero);

	//		if(m_coaches[i].m_runSoundID != INVALID_SOUNDID)
	//		{
	//			ISound *i_sound=m_coaches[i].m_pEntitySoundsProxy->GetSound(m_coaches[i].m_runSoundID);
	//			if(i_sound)
	//				i_sound->SetParam("speed",GetSpeedSoundParam(i), false);
	//		}
	//	}
	//}

}

void CFlowConvoyNode::SplitLineSound()
{
	//if(m_coaches[m_splitCoachIndex].m_pEntitySoundsProxy && m_coaches[m_splitCoachIndex].m_runSoundID == INVALID_SOUNDID)
	//{
	//	m_coaches[m_splitCoachIndex].m_runSoundID=m_engineStartSoundID=PlayLineSound(m_splitCoachIndex, "sounds/vehicles_exp1:train:run", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_Zero);
	//}
}

void CFlowConvoyNode::ConvoyStopSounds()
{
	//for (int i=0; i<m_coaches.size(); ++i)
	//{
	//	if(m_coaches[i].m_pEntitySoundsProxy && m_coaches[i].m_runSoundID != INVALID_SOUNDID)
	//		m_coaches[i].m_pEntitySoundsProxy->StopSound(m_coaches[i].m_runSoundID, ESoundStopMode_AtOnce);
	//	if(m_coaches[i].m_pEntitySoundsProxy && m_coaches[i].m_breakSoundID != INVALID_SOUNDID)
	//		m_coaches[i].m_pEntitySoundsProxy->StopSound(m_coaches[i].m_breakSoundID, ESoundStopMode_AtOnce);
	//	m_coaches[i].m_runSoundID = INVALID_SOUNDID;
	//	m_coaches[i].m_breakSoundID = INVALID_SOUNDID;
	//}
	//
	////train engine stop sound
	//m_coaches[0].m_pEntitySoundsProxy->PlaySound("sounds/vehicles_exp1:train:engine_stop", Vec3Constants<float>::fVec3_Zero, Vec3Constants<float>::fVec3_OneY, FLAG_SOUND_DEFAULT_3D, eSoundSemantic_Vehicle);
}

void CFlowConvoyNode::StopAllSounds()
{
	//for (int i=0; i<m_coaches.size(); ++i)
	//{
	//	if(m_coaches[i].m_pEntitySoundsProxy)
	//		m_coaches[i].m_pEntitySoundsProxy->StopAllSounds();
	//	m_coaches[i].m_runSoundID=INVALID_SOUNDID;
	//	m_coaches[i].m_breakSoundID=INVALID_SOUNDID;
	//}
	//m_hornSoundID=INVALID_SOUNDID;	
	//m_engineStartSoundID=INVALID_SOUNDID;
}

void CFlowConvoyNode::GetMemoryUsage(ICrySizer * s) const
{
	s->Add(*this);
}

std::vector<CFlowConvoyNode *> CFlowConvoyNode::gFlowConvoyNodes;

REGISTER_FLOW_NODE("Crysis2:Convoy", CFlowConvoyNode);
