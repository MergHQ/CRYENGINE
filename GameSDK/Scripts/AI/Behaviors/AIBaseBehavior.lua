local Behavior = CreateAIBehavior("AIBaseBehavior",
{
	OnLostSightOfTarget = function(behavior, entity, sender, data)
		if (entity.OnLostSightOfTarget) then
			entity:OnLostSightOfTarget(sender, data)
		end
	end,

	OnEnterCover = function(behavior, entity)
		entity:EnteredCover()
	end,

	OnLeaveCover = function(behavior, entity)
		entity:LeftCover()
	end,

	SetHoldGround = function(behavior, entity, sender, data)
		local stance = nil
		if (data.iValue == 1) then
			stance = STANCE_CROUCH
		end

		entity:SetHoldGround(data.point, data.point2, data.fValue, stance)
	end,

	CancelHoldGround = function(behavior, entity, sender, data)
		entity:CancelHoldGround()
	end,

	OnExecuteGoTo = function(behavior, entity)
		--entity:Log("GoTo Triggered!")
		AI.SetBehaviorVariable(entity.id, "GoTo", true)
	end,

	ACT_ANIMEX = function(behavior, entity, sender, data)
		behavior:SetAnimationData(entity, data)
		AI.SetBehaviorVariable(entity.id, "Animation", true)
	end,

	AnimationCanceled = function(behavior, entity, sender, data)
		AI.SetBehaviorVariable(entity.id, "Animation", false)
	end,

	SetAnimationData = function(behavior, entity, data)
		entity.AI.AnimationData = data
	end,

	OnNoPathFound = function(behavior, entity)
		entity:Log("AIBaseBehavior: Unhandled OnNoPathFound")
	end,

	SetAlertedStance = function(behavior, entity)
		AI.SetStance(entity.id, STANCE_ALERTED)
	end,

	-- List of Crysis2 game exclusions for participating in alerted response coordinations
	CanCoordinate = function(behavior, entity)
		if( AI.GetBehaviorVariable(entity.id, "InVehicle") ) then
			return false
		end

		if( AI.GetTypeOf(entity.id) == AIOBJECT_VEHICLE) then
			return false
		end

		return true
	end,

	OnExplosionDanger = function (behavior, entity, sender, data)
		if (behavior:ShouldReactToImmediateThreat(entity, sender, data)) then
			AI.SetBehaviorVariable(entity.id, "ImmediateThreat", true)
			entity.lastExplosiveThreatPos = data.point
			entity.lastExplosiveType = "explosive"
		end
	end,

	OnGrenadeDanger = function(behavior, entity, sender, data)
		if (behavior:ShouldReactToImmediateThreat(entity, sender, data)) then
			AI.SetBehaviorVariable(entity.id, "ImmediateThreat", true)
			entity.lastExplosiveThreatPos = data.point
			entity.lastExplosiveType = "grenade"
		end
	end,

	-- This function is called by Scorchers to warn friendlies of incoming fire.
	OnScorchDanger = function(behavior, entity, sender, data)
		if (behavior:ShouldReactToImmediateThreat(entity, sender, data)) then
			AI.SetBehaviorVariable(entity.id, "ImmediateThreatScorch", true)
			entity.lastExplosiveThreatPos = data.point
		end
	end,

	ShouldReactToImmediateThreat = function(behavior, entity, sender, data)
		-- Do not react if the agent is using a mounted weapon and not checking the conditions to leave the mounted weapon.
		if (entity.usingMountedWeapon and entity.AI.mountedWeaponCheck ~= nil and entity.AI.mountedWeaponCheck == false) then
			return false
		end

		return true
	end,

	SelectWeapon = function(behavior, entity)
		-- Do not select weapon, if is disabled
		if ( entity:IsActive() and AI.IsEnabled(entity.id) ) then
			entity:SelectPrimaryWeapon()
		end
	end,

	OnEnemySeen = function(behavior, entity, distance)
		if (entity.OnEnemySeen) then
			entity:OnEnemySeen(distance)
		end
	end,

})
