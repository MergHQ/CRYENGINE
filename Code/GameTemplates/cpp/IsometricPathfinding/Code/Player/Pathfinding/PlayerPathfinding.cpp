#include "StdAfx.h"
#include "PlayerPathFinding.h"

#include "Player/Player.h"
#include "Player/Input/PlayerInput.h"
#include "Player/Movement/PlayerMovement.h"

CPlayerPathFinding::CPlayerPathFinding()
	: m_movementRequestId(0)
	, m_pPathFollower(nullptr)
	, m_pFoundPath(nullptr)
{

}

CPlayerPathFinding::~CPlayerPathFinding()
{
	gEnv->pAISystem->GetMovementSystem()->UnregisterEntity(GetEntityId());

	SAFE_RELEASE(m_pPathFollower);
	SAFE_RELEASE(m_pFoundPath);
}

void CPlayerPathFinding::PostInit(IGameObject *pGameObject)
{
	m_pPlayer = static_cast<CPlayer *>(pGameObject->QueryExtension("Player"));

	m_navigationAgentTypeId = gEnv->pAISystem->GetNavigationSystem()->GetAgentTypeID("MediumSizedCharacters");

	m_callbacks.queuePathRequestFunction = functor(*this, &CPlayerPathFinding::RequestPathTo);
	m_callbacks.checkOnPathfinderStateFunction = functor(*this, &CPlayerPathFinding::GetPathfinderState);
	m_callbacks.getPathFollowerFunction = functor(*this, &CPlayerPathFinding::GetPathFollower);
	m_callbacks.getPathFunction = functor(*this, &CPlayerPathFinding::GetINavPath);

	gEnv->pAISystem->GetMovementSystem()->RegisterEntity(GetEntityId(), m_callbacks, *this);

	if (m_pPathFollower == nullptr)
	{
		PathFollowerParams params;
		params.maxAccel = m_pPlayer->GetCVars().m_moveSpeed;
		params.maxSpeed = params.maxAccel;
		params.minSpeed = 0.f;
		params.normalSpeed = params.maxSpeed;

		params.use2D = false;

		m_pPathFollower = gEnv->pAISystem->CreateAndReturnNewDefaultPathFollower(params, m_pathObstacles);
	}

	m_movementAbility.b3DMove = true;
}

void CPlayerPathFinding::RequestMoveTo(const Vec3 &position)
{
	CRY_ASSERT_MESSAGE(m_movementRequestId.id == 0, "RequestMoveTo can not be called while another request is being handled!");

	MovementRequest movementRequest;
	movementRequest.entityID = GetEntityId();
	movementRequest.destination = position;
	movementRequest.callback = functor(*this, &CPlayerPathFinding::MovementRequestCallback);
	movementRequest.style.SetSpeed(MovementStyle::Walk);

	movementRequest.type = MovementRequest::Type::MoveTo;

	m_state = Movement::StillFinding;

	m_movementRequestId = gEnv->pAISystem->GetMovementSystem()->QueueRequest(movementRequest);
}

void CPlayerPathFinding::RequestPathTo(MNMPathRequest &request)
{
	m_state = Movement::StillFinding;

	request.resultCallback = functor(*this, &CPlayerPathFinding::OnMNMPathResult);
	request.agentTypeID = m_navigationAgentTypeId;

	m_pathFinderRequestId = gEnv->pAISystem->GetMNMPathfinder()->RequestPathTo(this, request);
}

void CPlayerPathFinding::CancelCurrentRequest()
{
	CRY_ASSERT(m_movementRequestId.id != 0);

	gEnv->pAISystem->GetMovementSystem()->CancelRequest(m_movementRequestId);
	m_movementRequestId = 0;

	if (m_pathFinderRequestId != 0)
	{
		gEnv->pAISystem->GetMNMPathfinder()->CancelPathRequest(m_pathFinderRequestId);

		m_pathFinderRequestId = 0;
	}
}

void CPlayerPathFinding::OnMNMPathResult(const MNM::QueuedPathID& requestId, MNMPathRequestResult& result)
{
	m_pathFinderRequestId = 0;

	if (result.HasPathBeenFound())
	{
		m_state = Movement::FoundPath;

		SAFE_DELETE(m_pFoundPath);
		m_pFoundPath = result.pPath->Clone();

		// Bump version
		m_pFoundPath->SetVersion(m_pFoundPath->GetVersion() + 1);

		m_pPathFollower->Reset();
		m_pPathFollower->AttachToPath(m_pFoundPath);
	}
	else
	{
		m_state = Movement::CouldNotFindPath;
	}
}


Vec3 CPlayerPathFinding::GetVelocity() const
{
	return m_pPlayer->GetMovement()->GetVelocity();
}

void CPlayerPathFinding::SetMovementOutputValue(const PathFollowResult& result)
{
	float frameTime = gEnv->pTimer->GetFrameTime();
	float moveStep = m_pPlayer->GetCVars().m_moveSpeed * frameTime;

	auto &playerMovement = *m_pPlayer->GetMovement();

	if (result.velocityOut.GetLength() > moveStep)
	{
		playerMovement.RequestMove(result.velocityOut.GetNormalized() * moveStep);
	}
	else
	{
		playerMovement.SetVelocity(result.velocityOut);
	}
}

void CPlayerPathFinding::ClearMovementState()
{
	m_pPlayer->GetMovement()->SetVelocity(ZERO);
}