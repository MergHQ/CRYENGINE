-- Globals
TargetTypeNone = 0
TargetTypeInteresting = 1
TargetTypeThreatening = 2
TargetTypeMemory = 3
TargetTypeVisual = 4


AIBase =
{
	PropertiesInstance =
	{
	 	AITerritoryAndWave =
	 	{
   			aiterritory_Territory = "<None>",
   			aiwave_Wave           = "<None>",
 		},

		AI =
		{
			bHostileIfAttacked = 0,
			bGoBackToStartOnIdle = 0,
		},

		Interest =
	 	{
	 		bOverrideArchetype = 0,
			bInterested = 1,
			MinInterestLevel = 0.0,
			Angle = 270.0,
		},
		Readability =
	 	{
	 		bOverrideArchetype = 0,
	 		bIgnoreAnimations = 0,
	 		bIgnoreVoice = 0,
	 	},
	},

	Properties =
	{
		aibehavior_behaviour = "",
		aicharacter_character = "",
		esFaction = "Players",

		Perception =
		{
			cloakMaxDistCrouchedAndMoving = 4.0,
			cloakMaxDistCrouchedAndStill = 4.0,
			cloakMaxDistMoving = 4.0,
			cloakMaxDistStill = 4.0,
		},
	},

	BehaviorProperties =
	{
		ReactToSoundFromPotentialTargetTimeOut = 2.0,
	},

	melee =
	{
		angleThreshold = 40.0, -- Required angle between <body dir> and <direction to target>
	},

	AIMovementAbility =
	{
		avoidanceAbilities = AVOIDANCE_ALL,
		pushableObstacleWeakAvoidance = true,
		pushableObstacleAvoidanceRadius = 0.4,
	},

	Server =
	{
		OnStartGame = function(self)
			self:SetupTerritoryAndWave()
		end,

		OnHit = function(self, hit)
			if (self.PropertiesInstance.AI.bHostileIfAttacked and (tonumber(self.PropertiesInstance.AI.bHostileIfAttacked) ~= 0)) then
				if (hit.shooterId and (hit.shooterId ~= g_localActorId) and (not AI.Hostile(self.id, hit.shooterId))) then
					if ((hit.type ~= "collision") or (hit.damage > 50)) then
						AI.AddPersonallyHostile(self.id, hit.shooterId)
					end
				end
			end
			return BasicActor.Server.OnHit(self, hit)
		end
	},

	DefendPosition =
	{
		pos = {x=0, y=0, z=0},
		radius = 15,
	},

	SuspendGoTo = function(entity)
		if (entity.AI.GoToData) then
			entity:Log("Suspending goto")
			entity.AI.GoToData.Queued = true
			AI.SetBehaviorVariable(entity.id, "GoTo", false)
		end
	end,

	ResumeGoTo = function(entity)
		if (entity.AI.GoToData and entity.AI.GoToData.Queued) then
			entity:Log("Resuming goto")
			entity.AI.GoToData.Queued = nil
			AI.SetBehaviorVariable(entity.id, "GoTo", true)
		end
	end,

	OnActionStart = function(entity)
		entity:Log("OnActionStart")
		entity:SuspendGoTo(entity)
	end,

	OnActionEnd = function(entity)
		entity:Log("OnActionEnd")
		entity:ResumeGoTo(entity)
	end,
	
	EnableBehaviorTreeEvaluation = function(entity)
		AI.SetBehaviorTreeEvaluationEnabled(entity.id, true)
	end,
	
	DisableBehaviorTreeEvaluation = function(entity)
		AI.SetBehaviorTreeEvaluationEnabled(entity.id, false)
	end,
}

--------------------------------------------------
-- Utils
--------------------------------------------------

function AIBase:StartOrRestartPipe(goalPipeName)
	self:SelectPipe(0, goalPipeName, 0, 0, 1)
end

function AIBase:IsUsingSecondaryWeapon()
	return (self:CheckCurWeapon() == 1) ;
end

function AIBase:GetTargetType(targetType, threat)

	local resultType = TargetTypeNone;

	if (targetType == AITARGET_MEMORY) then
		resultType = TargetTypeMemory;
	elseif (targetType == AITARGET_SOUND) then
		if (threat == AITHREAT_INTERESTING) then
			resultType = TargetTypeInteresting;
		elseif (threat == AITHREAT_THREATENING) then
			resultType = TargetTypeThreatening;
		end
	elseif (targetType == AITARGET_VISUAL) then
		resultType = TargetTypeVisual;
	end

	return resultType;

end


function AIBase:GetDistanceToSearchTarget()

	-- This function is not used yet, but should be used in search behaviors to simplify code.

	local targetType = AI.GetTargetType(self.id);
	local dist;
	if (targetType == TARGET_NONE) then
		-- Beacon
		local beaconPosition = g_Vectors.temp_v1;
		AI.GetBeaconPosition(self.id, beaconPosition);
		dist = DistanceVectors(self:GetPos(), beaconPosition);
	else
		-- Attention target
		dist = AI.GetAttentionTargetDistance(self.id);
	end

	if (dist == nil) then
		Log("dist is nil");
	end

	return dist;

end


function AIBase:IsTargetCloaked()

	local target = AI.GetAttentionTargetEntity(self.id)
	local targetCloaked = nil;
	if (target) then
		targetCloaked = AI.GetParameter(target.id, AIPARAM_CLOAKED);
	end

	return targetCloaked;

end

--------------------------------------------------
-- Debug
--------------------------------------------------

function AIBase:Log(message, ...)
	local ai_DebugDraw = System.GetCVar("ai_DebugDraw")
	if (ai_DebugDraw == 1) then
		Log("%s: %s", EntityName(self), string.format(message, ...))
	end
end

function AIBase:Warning(message, ...)
	Log("[Warning] %s: %s", EntityName(self), string.format(message, ...))
end

function AIBase:Trace(message, ...)
	if (self.traceMe) then
		Log("%s: %s", EntityName(self), string.format(message, ...))
	end
end

function AIBase:LogAlways(message, ...)
	Log("%s: %s", EntityName(self), string.format(message, ...))
end

function AIBase:OnError(errorMessage, ...)

	local ai_DebugVisualScriptErrors = System.GetCVar("ai_DebugVisualScriptErrors")
	if (ai_DebugVisualScriptErrors == 1) then
		self:DrawSlot(0, 0);
		self:LoadObject(1, "objects/characters/animals/dog/dog_02.cgf")
		self:DrawSlot(1, 1)
		self:SetSlotScale(1, 4)
		if (self.dogTimer) then
			Script.KillTimer(self.dogTimer);
		end
		self.dogTimer = Script.SetTimerForFunction(6000, "AIBase.OnDogTimer", self);
	end

	if (errorMessage) then
		Log("%s: %s", EntityName(self), string.format(errorMessage, ...));
	end

end

-- Error is just an alias for OnError
AIBase.Error = AIBase.OnError

function AIBase:OnDogTimer()

	self:DrawSlot(1, 0);
	self:FreeSlot(1);
	self:DrawSlot(0, 1);

end

--------------------------------------------------
-- Perception
--------------------------------------------------

function AIBase:TargetLost()

	AI.Signal(SIGNALFILTER_SENDER, 1, "OnTargetLost", self.id);

end


function AIBase:OnNoTarget()

end

function AIBase:TargetNotVisible()
	AI.Signal(SIGNALFILTER_SENDER, 1, "OnTargetNotVisible", self.id);
end

function AIBase:GetTimeSinceTargetLastSeen(targetEntity)

	local targetType = self:GetTargetTypeForComm(targetEntity)

	local lastSeenTime = self.AI.lastTimeTargetWasSeen;

	if( targetType ~= nil ) then
		local blackboard = self:GetGroupBlackboard();
		local lastReported = targetType .. "LastSeen"
		lastSeenTime = blackboard[lastReported];
	end

	--If we haven't seen this type before, then return a negative value
	if(lastSeenTime == nil) then
		return -1
	--If it is still in sight return 0
	elseif (lastSeenTime == -1) then
		return 0
	--Otherwise report how long its been since last sighting of this targetType
	else
		return _time - lastSeenTime;
	end
end

function AIBase:AcknowledgeEnemy(targetEntity)
	local blackboard = self:GetGroupBlackboard();

	local targetType = self:GetTargetTypeForComm(targetEntity)
	if( targetType ~= nil ) then
		local entryName = targetType .. "Acknowledged"
		blackboard[entryName] = true;
	end
end

function AIBase:IsEnemyAcknowledged(targetEntity)
	local blackboard = self:GetGroupBlackboard();

	local targetType = self:GetTargetTypeForComm(targetEntity)
	if( targetType ~= nil ) then
		local entryName = targetType .. "Acknowledged"
		local isAcknowledged = blackboard[entryName];
		if(not isAcknowledged) then
			return false
		else
			return true
		end
	end

	return true
end

function AIBase:OnEnemySeen()
	local blackboard = self:GetGroupBlackboard();

	local attentionTarget = AI.GetAttentionTargetEntity(self.id);

	if (not self:ValidateAttentionTarget(attentionTarget)) then
		return;
	end

	local targetType = self:GetTargetTypeForComm(attentionTarget)
	if( targetType ~= nil ) then
		local lastReported = targetType .. "LastSeen"
		blackboard[lastReported] = -1;
	end

	self.AI.lastTimeTargetWasSeen = -1;
	self:ResetTargetLostTimer();

	self:SafeKillTimer(self.targetNotVisibleTimer);

end

function AIBase:GetTargetTypeForComm(targetEntity)
	local targetType = nil

	if (targetEntity) then
		attTargetClassName = targetEntity.class

		if(attTargetClassName == "HumanGrunt") then
			targetType = "Marine"
		elseif (targetEntity.actor and targetEntity.actor:IsPlayer()) then
			targetType = "Player"
		end
	end

	return targetType
end

function AIBase:OnLostSightOfTarget()

	local blackboard = self:GetGroupBlackboard();

	local attentionTarget = AI.GetAttentionTargetEntity(self.id);

	if (not self:ValidateAttentionTarget(attentionTarget)) then
		return;
	end

	local targetType = self:GetTargetTypeForComm(attentionTarget)
	if( targetType ~= nil ) then
		local lastReported = targetType .. "LastSeen"
		blackboard[lastReported] = _time;
	end

	self.AI.lastTimeTargetWasSeen = _time;
	self.AI.lastKnownTargetPosition = nil;
	local targetEntity = AI.GetAttentionTargetEntity(self.id)
	if (targetEntity) then
		self.AI.lastKnownTargetPosition = targetEntity:GetWorldPos()
	end
	self:SafeKillTimer(self.targetLostTimer);
	
	if (self.BehaviorProperties and self.BehaviorProperties.fTimeUntilSearchInCombat) then
		self.targetLostTimer = Script.SetTimer(self.BehaviorProperties.fTimeUntilSearchInCombat * 1000, function() self:TargetLost() end)
	end

	self:SafeKillTimer(self.targetNotVisibleTimer);
	self.targetNotVisibleTimer = Script.SetTimer(2000, function() self:TargetNotVisible() end);

end


function AIBase:OnGroupEnemySeen()
end


function AIBase:OnGroupUnderAttack()
end


function AIBase:ResetTargetLostTimer()

	self:SafeKillTimer(self.targetLostTimer);

end


function AIBase:OnNewAttentionTarget(sender, data)

end


function AIBase:OnAttentionTargetThreatChanged(sender, data)

end

--------------------------------------------------
-- Misc
--------------------------------------------------

function AIBase:SetGoToData(data)
	--self:Log("GoTo data set")
	self.AI.NextGoToData = data
end

function AIBase:SetGoToPosition(data)
	--self:Log("GoTo data set")
	if(self.AI.GoToData) then
		self.AI.GoToData.Position = data.Position
	else
		entity:LogAlways("Trying to set go data position with no current go to data")
	end
end

function AIBase:ShouldReactToSoundFromPotentialTarget(point)

	-- Too soon after the last sound reaction?
	if (self.lastReactionToSoundFromPotentialTargetTime and self.lastReactionToSoundFromPotentialTargetTime + self.BehaviorProperties.ReactToSoundFromPotentialTargetTimeOut > _time) then
		return false;
	end

	-- Sound point already approximatively visible in agent's view cone?
	if (point) then
		local agentToPointDir = g_Vectors.temp_v1;
		SubVectors(agentToPointDir, point, self:GetWorldPos());
		local dotProduct = dotproduct3d(agentToPointDir, self:GetDirectionVector());
		local soundIsInFront = dotProduct > 0.0;
		if (soundIsInFront) then
			return false;
		end
	end

	-- React to sound!
	self.lastReactionToSoundFromPotentialTargetTime = _time;
	return true;

end


function AIBase:LeaveCover()

	AI.SetInCover(self.id, false);

end


function AIBase:SetupTerritoryAndWave(territory, wave)

	if (territory and territory ~= "") then
		self.PropertiesInstance.AITerritoryAndWave.aiterritory_Territory = territory;
	end

	if (wave and wave ~= "") then
		self.PropertiesInstance.AITerritoryAndWave.aiwave_Wave = wave;
	end

	AI_Utils:SetupTerritory(self);
	AI_Utils:SetupStandby(self);

end


function AIBase:DropBeacon()

	self:TriggerEvent(AIEVENT_DROPBEACON);

end


function AIBase:SendGroupSignal(notification, sender, data)

	AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, notification, self.id, data);

end


function AIBase:AssaultTarget(target)

	if (target) then
		if (not self.assaultTarget) then
			self.assaultTarget = {x=0, y=0, z=0};
		end
		CopyVector(self.assaultTarget, target);
	end

end


function AIBase:SafeKillTimer(timer)

	if (timer) then
		Script.KillTimer(timer);
	end

end


function AIBase:SetDefendPosition(target, radius)
	if (target) then
		CopyVector(self.DefendPosition.pos, target)
		if(radius > 1) then
			self.DefendPosition.radius = radius
		else
			self.DefendPosition.radius = 15 -- default
		end
		AI.SetBehaviorVariable(self.id, "DefendPosition", true)
	else
		Log("SetDefendTarget failed.")
	end
end

function AIBase:CancelDefendPosition()
	AI.SetBehaviorVariable(self.id, "DefendPosition", false)
end

function AIBase:SetHoldGround(position, direction, skipCover, stance)
	if (self.AI.HoldGround == nil) then
		self.AI.HoldGround = {
			pos = {x=0, y=0, z=0},
			dir = {x=0, y=0, z=0},
			radius = 5,
			skipCover = false
		}
	end

	if (IsNotNullVector(position)) then
		CopyVector(self.AI.HoldGround.pos, position)
	else
		Log("SetHoldGround failed.")
		return
	end

	if(IsNotNullVector(direction)) then
		CopyVector(self.AI.HoldGround.dir, direction)
	end

	self.AI.HoldGround.skipCover = (skipCover > 0)
	self.AI.HoldGround.stance = stance

	self:LeaveCover()
	AI.SetBehaviorVariable(self.id, "AtHoldGroundPos", false)
	AI.SetBehaviorVariable(self.id, "HoldGround", true)
end

function AIBase:CancelHoldGround()
	AI.SetBehaviorVariable(self.id, "HoldGround", false)
end

function AIBase:OnGroupMemberDiedWithinCommRange(sender, data)

	local killedEntity = System.GetEntity(data.id);

end


function AIBase:PerformBulletReaction()

	if (not AI.IsTakingCover(self.id, 7.5)) then
		if (not self:IsUsingPipe("BulletReaction")) then -- avoid performing it twice
			self:InsertSubpipe(0, "BulletReaction");
		end
	end

end


function AIBase:GetGroupBlackboard()

	local groupID = AI.GetGroupOf(self.id);
	AI_Utils:VerifyGroupBlackBoard(groupID);
	return AIBlackBoard[groupID];

end


function AIBase:ShouldGoBackToStartOnIdle()
	return self.AI.bGoBackToStartOnIdle and self.lastIdlePosition;
end

function AIBase:EnteredCover()
	self.AI.inCoverStartTime = _time
	self.AI.outOfCoverStartTime = nil
end

function AIBase:LeftCover()
	self.AI.inCoverStartTime = nil
	self.AI.outOfCoverStartTime = _time
end

function AIBase:GetTimeInCover()
	if (self.AI.inCoverStartTime == nil) then
		return 0
	end

	return _time - self.AI.inCoverStartTime
end

function AIBase:GetTimeOutOfCover()
	if (self.AI.outOfCoverStartTime == nil) then
		return 0
	end

	return _time - self.AI.outOfCoverStartTime
end

function AIBase:IsTargetVisual()
	local targetType = AI.GetTargetType(self.id);

	return targetType == AITARGET_VISUAL or targetType == AITARGET_ENEMY
end

function AIBase:ValidateAttentionTarget(data)
	-- Check if the result from GetAttentionTarget script bind is valid.
	-- Due to the current update order of the AI system, an AI agent can suddently
	-- lose its attention target without updating immediately the current behavior,
	-- leading to errors if the behavior happens to rely on the existence of an
	-- attention target, like most combat behaviors do. /Mario 11-12-2010
	if (data == nil) then
		self:Log("AttentionTarget is unexpectedly invalid!")
		return false
	else
		return true
	end
end

function AIBase:CallBehaviorFunction(f, ...)
	--self:Log("#### CallBehaviorFunction - "..f.. "####")
	local behavior = self.Behavior
	if (behavior and behavior[f]) then
		behavior[f](behavior, self, ...)
	end
end
