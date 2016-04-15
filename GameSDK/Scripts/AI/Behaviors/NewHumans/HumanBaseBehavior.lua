local Behavior = CreateAIBehavior("HumanBaseBehavior", "AIBaseBehavior",
{
	Constructor = function (self, entity)
		entity:SelectPipe(0, "do_nothing");
	end,

	Destructor = function(self, entity)
	end,

	OnBulletRain = function(self, entity, sender, data)
		entity.lastBulletRainTime = _time
		entity:DropBeacon();

		local groupCount = AI.GetGroupCount(entity.id);
		if(groupCount < 2) then
			AI.PlayCommunication(entity.id, "bullet_rain_alone", "Personal", 0.5);
		else
			AI.PlayCommunication(entity.id, "bullet_rain", "BulletRain", 0.5);
		end
		if (entity.OnBulletRain) then
			entity.OnBulletRain(entity, sender, data);
		end
	end,

	OnEnemyDamage = function(self, entity, sender, data)
		entity:DropBeacon();
		--entity:SendGroupSignal("OnGroupUnderAttack", sender, data);

		if (entity.OnEnemyDamage) then
			entity.OnEnemyDamage(entity, sender, data);
		end
	end,

	OnEnemySeen = function(self, entity, distance)
		entity:DropBeacon();
		entity:SendGroupSignal("OnGroupEnemySeen", sender, data);

		local attentionTarget = AI.GetAttentionTargetEntity(entity.id);	

		if (not entity:ValidateAttentionTarget(attentionTarget)) then
			return;
		end

		-- Readability if the target is still cloaked
		local timeLastSeen = entity:GetTimeSinceTargetLastSeen(attentionTarget)
		if (entity:IsTargetCloaked() and (timeLastSeen < 0 or timeLastSeen > 5) ) then
			if (distance < 5) then
				AI.PlayCommunication(entity.id, "comm_surprised", "Reaction", 0.5);
			else
				AI.PlayCommunication(entity.id, "surprised", "Reaction", 0.5);
			end
		end

		if (entity.OnEnemySeen) then
			entity:OnEnemySeen(distance);
		end
	end,

	OnThrowSmokeGrenade = function(self, entity, sender, data)
		local targetPos = data.point;
		if (data.id) then
			local target = System.GetEntity(data.id);
			if (target) then
				targetPos = target:GetPos();
			end
		end

		if (targetPos) then
			entity:ThrowSmokeGrenade(targetPos, data.iValue);
		end
	end,

	OnCallReinforcements = function(self, entity, sender, data)
		entity.reinforcementSpotId = data.id;
		entity.reinforcementSpotType = data.iValue;
		AI.SetBehaviorVariable(entity.id, "CallReinforcements", true)
	end,

	OnThreateningSoundHeard = function(self, entity, distanceFromTarget)
		AI.SetAlarmed(entity.id)
		entity:DropBeacon()
	end,

	OnGroupEnemySeen = function(self, entity, sender, data)
		AI.SetAlarmed(entity.id);

		if (entity.OnGroupEnemySeen) then
			entity:OnGroupEnemySeen(sender, data);
		end
	end,

	OnGroupUnderAttack = function(self, entity, sender, data)
		AI.SetAlarmed(entity.id);

		if (entity.OnGroupUnderAttack) then
			entity:OnGroupUnderAttack(sender, data);
		end
	end,


	SetAssaultTarget = function(self, entity, sender, data)
		entity:AssaultTarget(data.point);
	end,

	SetDefendTarget = function(self, entity, sender, data)
		entity:SetDefendPosition(data.point, data.fValue)
	end,

	CancelDefendTarget = function(self, entity, sender, data)
		entity:CancelDefendPosition()
	end,

	OnLostSightOfTarget = function(self, entity, sender, data)
		if (entity.OnLostSightOfTarget) then
			entity:OnLostSightOfTarget(sender, data);
		end
	end,


	OnNoTarget = function(self, entity, sender, data)
		if (entity.OnNoTarget) then
			entity:OnNoTarget(sender, data);
		end
	end,


	OnEnemyMemory = function(self, entity, distance)
		if (entity.OnEnemyMemory) then
			entity:OnEnemyMemory(distance);
		end

		-- if (entity:CanThrowGrenade()) then
			--entity:ThrowGrenade();
		-- end
	end,


	OnNewAttentionTarget = function(self, entity, sender, data)
		if (entity.OnNewAttentionTarget) then
			entity:OnNewAttentionTarget(sender, data);
		end

		-- Personal targets are pulsed so the AI favors them briefly
		local targetType = data.iValue;
		local targetThreat = data.iValue2;
		local bIsFromGroup = (data.fValue > 0);
		if (targetType == AITARGET_VISUAL and targetThreat == AITHREAT_AGGRESSIVE and not bIsFromGroup) then
			AI.TriggerCurrentTargetTrackPulse(entity.id, "VisualPrimary", "Stick")
			
			--Always play contact readability here?
			self:PlayContactReadability(entity);
		end

	end,


	OnAttentionTargetThreatChanged = function(self, entity, sender, data)
		if (entity.OnAttentionTargetThreatChanged) then
			entity:OnAttentionTargetThreatChanged(sender, data);
		end
	end,

	-- Melee action initiated
	OnMeleeExecuted = function(self, entity)
		AI.PlayCommunication(entity.id, "combat_melee", "Personal", 0.25);
	end,

	OnGroupMemberDiedWithinCommRange = function(self, entity, sender, data)
		if (entity.OnGroupMemberDiedWithinCommRange) then
			entity:OnGroupMemberDiedWithinCommRange(sender, data);
		end
	end,

	OnDeadMemberSpotted = function(self, entity, sender, data)
		entity:Log("OnDeadMemberSpotted (HumanBaseBehavior)")
		AI.PlayCommunication(entity.id, "alert_radio_death_confirm", "ImmeadiateThreat", 2.0)

		--local victimID = data.id;
		AI.Signal(SIGNALFILTER_GROUPONLY_EXCEPT, 1, "OnDeadMemberSpottedBySomeoneElse", entity.id, data)
	end,

	OutputDeathDebugInfo = function(self, entity, witness, awareOfThreat)
		entity:Log("---------------------------");
		entity:Log("Member Death Information:");

		local answer;

		if (witness) then answer = "Yes!" else answer = "No" end
		entity:Log("- Witness: %s", answer);

		if (awareOfThreat) then answer = "Yes!" else answer = "No" end
		entity:Log("- Aware of threat: %s", answer);
	end,

	OnOutOfAmmo = function(self, entity, sender, data)
		AI.Signal(SIGNALFILTER_SENDER, 1, "LowAmmoStart", entity.id);
	end,

	OnLowAmmo = function(self, entity, sender, data)
		AI.Signal(SIGNALFILTER_SENDER, 1, "LowAmmoStart", entity.id);
	end,

	OnReloaded = function(self, entity, sender, data)
		AI.Signal(SIGNALFILTER_SENDER, 1, "LowAmmoFinished", entity.id);
	end,


	OnTargetArmoredHit = function(self, entity, sender, data)
		local isVisible = (data.fValue > 0);
		local targetThreat = data.iValue2;
		if (targetThreat == AITHREAT_AGGRESSIVE and isVisible) then
			if ( data.fValue < 30 ) then
				AI.PlayCommunication(entity.id, "comm_player_armor", "Reaction", 1.25);
			end
		end
	end,

	OnTargetCloaked = function(self, entity, sender, data)
		local isVisible = (data.fValue > 0);
		local targetThreat = data.iValue2;
		if (targetThreat == AITHREAT_AGGRESSIVE and isVisible) then
			if ( data.fValue < 30 ) then
				Script.SetTimer(System.GetCVar("ai_CompleteCloakDelay") * 1000, function() entity.Behavior:OnCloakDelayDone(entity); end);
			end
		end
	end,

	OnCloakDelayDone = function(self, entity)
		--Ensure again if the target is still cloaked
		if( entity:IsTargetCloaked()) then
			AI.PlayCommunication(entity.id, "comm_player_cloaks", "Reaction", entity.ReadabilityProperties.playerCloakReactionTimeout);
		end
	end,

	OnTargetUncloaked = function(self, entity, sender, data)
		local isVisible = (data.fValue > 0);
		if (isVisible) then
			if ( data.fValue < 30 ) then
				local target = System.GetEntity(data.id)			
				if(target) then
					local timeLastSeen = entity:GetTimeSinceTargetLastSeen(target)
					if (timeLastSeen < 0 or timeLastSeen > 2) then
						if(data.fValue < 10) then
							--Check angle to target
							local target = System.GetEntity(data.id)
							if target then
								local dirToTarget = g_Vectors.temp_v3;						
								SubVectors(dirToTarget, target:GetWorldPos(), entity:GetWorldPos() );
								NormalizeVector(dirToTarget);

								local dotProduct = dotproduct2d(dirToTarget, entity:GetDirectionVector(1));
								
								-- If within 30 degrees, then play animated version
								if dotProduct > 0.82 then
									AI.PlayCommunication(entity.id, "comm_player_uncloaks_nearby", "Reaction", entity.ReadabilityProperties.playerCloakReactionTimeout);
									return
								end
							end
						end
						Script.SetTimer(random(250,750), function() entity.Behavior:OnUncloakDelayDone(entity); end);
					end
				end
			end
		end
	end,

	OnUncloakDelayDone = function(self, entity)
		AI.PlayCommunication(entity.id, "comm_player_uncloaks", "Reaction", entity.ReadabilityProperties.playerCloakReactionTimeout);
	end,

	EnableIdleBreakAnimations = function(self, entity)
		entity:SetIdleBreakAnimations(true)
	end,

	CancelIdleBreakAnimations = function(self, entity)
		entity:SetIdleBreakAnimations(false)
	end,

	SetFlank = function(self, entity)
		entity:SetIsFlanker(true);
		AI.Signal(SIGNALFILTER_SENDER, 1, "Relocate", entity.id);
	end,

	CancelFlank = function(self, entity)
		entity:SetIsFlanker(false);
		AI.Signal(SIGNALFILTER_SENDER, 1, "Relocate", entity.id);
	end,

	SetSniperBehavior = function(self, entity)
		entity:SetIsSniper(true);
		AI.Signal(SIGNALFILTER_SENDER, 1, "Relocate", entity.id);
	end,

	CancelSniperBehavior = function(self, entity)
		entity:SetIsSniper(false);
		AI.Signal(SIGNALFILTER_SENDER, 1, "Relocate", entity.id);
	end,

	CheckDistance = function(self, entity)
		local targetDistance = AI.GetAttentionTargetDistance(entity.id) or -1;

		if(targetDistance < 0) then
			return;
		end

		if (targetDistance < entity:GetCloseCombatRange()) then
			--entity:Log("CheckDistance: AtCloseRange");
			AI.Signal(SIGNALFILTER_SENDER, 1, "AtCloseRange", entity.id);
		else
			--entity:Log("CheckDistance: AtOptimalRange");
			AI.Signal(SIGNALFILTER_SENDER, 1, "AtOptimalRange", entity.id);
		end
	end,

	PrepareForMountedWeaponUse = function(self, entity, sender, data)
		entity:PrepareForMountedWeaponUse(data.id, data.iValue);
	end,

	ReachedMountedWeapon = function(self, entity, sender, data)
		entity:Log("Reached mounted weapon");
		entity:SelectPipe(0, "Empty")
		AI.Signal(SIGNALFILTER_SENDER, 1, "UseMountedWeapon", entity.id);
	end,

	FailedToReachMountedWeapon = function(self, entity, sender, data)
		entity:Log("Failed to reached mounted weapon");
		entity:SelectPipe(0, "Empty")
	end,

	TurnOnMountedWeaponCheck = function(behavior, entity)
		entity:Log("TurnOnMountedWeaponCheck received... turning on mounted weapon check")
		entity.AI.mountedWeaponCheck = true
	end,

	TurnOffMountedWeaponCheck = function(behavior, entity)
		entity:Log("TurnOffMountedWeaponCheck received... turning off mounted weapon check")
		entity.AI.mountedWeaponCheck = false
	end,

	OnEnterCloseRange = function(behavior, entity)
		AI.SetBehaviorVariable(entity.id, "AtCloseRange", true)

		-- TODO : This group melee mutex should not be implemented over the close range signals, 
		-- which makes it very unreliable and easy to break...
		-- Instead, the "melee" behavior itself should lock the melee for the other group members. /Mario
		if (entity:AcquireGroupMeleeMutex()) then
			AI.SetBehaviorVariable(entity.id, "AtCloseRangeWithGroupMeleeMutex", true);
		end
	end,

	OnLeaveCloseRange  = function(behavior, entity)
		entity:ReleaseGroupMeleeMutex();
		AI.SetBehaviorVariable(entity.id, "AtCloseRange", false)
		AI.SetBehaviorVariable(entity.id, "AtCloseRangeWithGroupMeleeMutex", false)
	end,

	TryToSideStepFromBulletRain = function(behavior, entity, bulletRainData)
		canSideStep = entity:CanSideStepFromBulletRain(bulletRainData)
		if (canSideStep.result == true) then
			behavior:DoSideStep(entity, canSideStep.side)
			return true
		end
		return false
	end,

	DoSideStep = function(behavior, entity, sideStep)
		entity.lastSideStepTime = _time
		if (sideStep == SIDESTEP_LEFT) then
			entity:SelectPipe(0, "SideStepLeft")
		else
			entity:SelectPipe(0, "SideStepRight")
		end
	end,
	
	----------------------------

	PlayFirstContactReadability = function(behavior, entity)
		local attentionTarget = AI.GetAttentionTargetEntity(entity.id);

		if (not entity:ValidateAttentionTarget(attentionTarget)) then
			return;
		end

		attTargetClassName = attentionTarget.class

		local coordinations = {ContactMarineFirst=true, ContactPlayerFirst=true}

		local filterFunction = function(behavior, entity)
			return (AI.GetAttentionTargetDistance(entity.id) > 10)
		end
		
		if (attentionTarget.actor and attentionTarget.actor:IsPlayer() ) then
			local groupId = AI.GetGroupOf(entity.id)
			if(groupId and AI.GetGroupCount(groupId) > 1) then
				StartCoordinatedReadability(behavior, entity, "ContactPlayerFirst", "comm_combat_contact_player_close", "Confirmed", coordinations, filterFunction);				
			else
				AI.PlayCommunication(entity.id, "comm_combat_contact_player_close", "Confirmed", 1.0)
			end
		else
			StartCoordinatedReadability(behavior, entity, "ContactMarineFirst", "comm_combat_contact_marine_first", "Confirmed", coordinations, filterFunction);
		end
	end,
	
	PlayContactReadability = function(behavior, entity)
		local attentionTarget = AI.GetAttentionTargetEntity(entity.id);	

		if (not entity:ValidateAttentionTarget(attentionTarget)) then
			return;
		end

		-- Ignore any contact readabilities while the target is cloaked
		if (entity:IsTargetCloaked()) then
			return;
		end

		attTargetClassName = attentionTarget.class
		
		local targetType = nil
		if(attTargetClassName == "HumanGrunt") then
			targetType = "Marine"
		elseif (attentionTarget.actor and attentionTarget.actor:IsPlayer()) then
			targetType = "Player"
		end

		local timeLastSeen = entity:GetTimeSinceTargetLastSeen(attentionTarget)	

		if(targetType and timeLastSeen > 1.0) then
			--Check that its been some time since the group has responded to a new target		

			if (targetType == "Marine") then
				if(timeLastSeen >= 3) then			
					AI.PlayCommunication(entity.id, "combat_contact_marine", "Confirmed", 2);
				end
			elseif (targetType == "Player" and timeLastSeen >= 2.5) then

				zDistanceToPlayer = attentionTarget:GetPos().z - entity:GetPos().z;

				-- Play detailed ones all the time, since they are more intersting
				if (attentionTarget.actor:GetLinkedVehicleId() ~= nil) then
					AI.PlayCommunication(entity.id, "combat_contact_player_car", "Confirmed", 2);	-- TODO: Filter for type
				elseif (zDistanceToPlayer > 2) then
					AI.PlayCommunication(entity.id, "combat_contact_player_higher", "Confirmed", 2);
				elseif (zDistanceToPlayer < -2) then
					AI.PlayCommunication(entity.id, "combat_contact_player_lower", "Confirmed", 2);
				elseif (attentionTarget:GetSpeed() > 5) then
					AI.PlayCommunication(entity.id, "combat_contact_player_moving", "Confirmed", 2);
				else			
					if(timeLastSeen >= 4.5) then								
						local groupCount = AI.GetGroupCount(entity.id);
						if(groupCount < 2) then
							AI.PlayCommunication(entity.id, "combat_contact_player_alone", "Confirmed", 2);
						else
							AI.PlayCommunication(entity.id, "combat_contact_player", "Confirmed", 2);
						end
					end
				end
			end
		end
	end,

	ShouldFireRocket = function(behavior, entity)
		return (entity:IsRocketeer() and entity:CanFireJAW())
	end,

})
