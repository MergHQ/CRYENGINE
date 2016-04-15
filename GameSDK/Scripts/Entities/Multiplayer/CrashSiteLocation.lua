----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Crash site location entity (crash site mode)
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 17:04:2009   16:00 : Created by Colin Gulliver
--
----------------------------------------------------------------------------------------------------

Script.ReloadScript("Scripts/Entities/AlienTech/AlienTech.lua");

CrashSiteLocation = new (AlienTech);

CrashSiteLocation.Properties.ControlRadius = 8;
CrashSiteLocation.Properties.ControlHeight = 5;
CrashSiteLocation.Properties.ControlOffsetZ = 0;
CrashSiteLocation.Properties.UseFlowGraph = 0;
CrashSiteLocation.Properties.DebugDraw = 0;
CrashSiteLocation.Properties.radiusEffectScale = 1.0;

CrashSiteLocation.SmokeSlot = -1;
CrashSiteLocation.AlertActive = false;
CrashSiteLocation.IsActive = false;
CrashSiteLocation.StartPos = nil;
CrashSiteLocation.Velocity = nil;
CrashSiteLocation.Falling = false;
CrashSiteLocation.Acceleration = nil;
CrashSiteLocation.DropPos = nil;

CrashSiteLocation.PreviousColour = { x = 0.0, y = 0.0, z = 0.0,	};
CrashSiteLocation.TargetColour = { x = 0.0, y = 0.0, z = 0.0, };

CrashSiteLocation.ColourChangePerc = 1.0;
CrashSiteLocation.InDeathAnim = false;
CrashSiteLocation.TimeToDeath = 0.0;
CrashSiteLocation.InFlowGraph = false;
CrashSiteLocation.HasReceivedIconEvent = false;
CrashSiteLocation.NumTimesLanded = 0;

----------------------------------------------------------------------------------------------------
CrashSiteLocation.START_FALL_HEIGHT = 300;
CrashSiteLocation.ICON_HEIGHT = 0.15;

CrashSiteLocation.PARTICLE_EFFECT_TRAIL      = "cw2_gameplay.crash_site.drop_pod_trail";
CrashSiteLocation.PARTICLE_EFFECT_CRASH      = "cw2_gameplay.crash_site.drop_pod_impact";
CrashSiteLocation.PARTICLE_EFFECT_EXPLOSION  = "cw2_gameplay.crash_site.drop_pod_explode";
CrashSiteLocation.PARTICLE_EFFECT_DEATH_ANIM = "cw2_gameplay.crash_site.drop_pod_explode_builup";

CrashSiteLocation.DEATH_ANIM_LENGTH = 5.0;

CrashSiteLocation.AUDIO_START = GameAudio.GetSignal("Crashsite_Start");
CrashSiteLocation.AUDIO_LAND = GameAudio.GetSignal("Crashsite_Land");
CrashSiteLocation.AUDIO_VOLATILE = GameAudio.GetSignal("Crashsite_Volatile");
CrashSiteLocation.AUDIO_EXPLODE = GameAudio.GetSignal("Crashsite_Explode");

CrashSiteLocation.NEUTRAL_GAMESTATE_STRING = "@ui_msg_cs_status_0"

CrashSiteLocation.EXPLOSION_INFO = {
	damage = 1000.0,
	pressure = 2000.0,
	hole_size = 3.0,
	type = "alienDropPodBounce",
	min_radius = 2.0,
	max_radius = 5.0,
	min_phys_radius = 2.2,
	max_phys_radius = 4.0,
};

-- Second, smaller explosion when crash-site is removed
CrashSiteLocation.EXPLOSION_INFO_2 = {
	damage = 100.0,
	pressure = 1000.0,
	hole_size = 1.0,
	type = "alienDropPodBounce",
	min_radius = 1.0,
	max_radius = 2.5,
	min_phys_radius = 1.2,
	max_phys_radius = 3.0,
};

----------------------------------------------------------------------------------------------------
function CrashSiteLocation.Server:OnInit()
	--Log("CrashSiteLocation.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation.Client:OnInit()
	--Log("CrashSiteLocation.Client.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:OnReset()
	--Log("CrashSiteLocation.OnReset, isActive=%s, isEditor=%s", tostring(self.IsActive), tostring(System.IsEditor()));
	self:CreateRenderProxy();
	self:ClearSmoke();

	self:PreLoadParticleEffect(self.PARTICLE_EFFECT_TRAIL);
	self:PreLoadParticleEffect(self.PARTICLE_EFFECT_CRASH);
	self:PreLoadParticleEffect(self.PARTICLE_EFFECT_EXPLOSION);
	self:PreLoadParticleEffect(self.PARTICLE_EFFECT_DEATH_ANIM);

	if (self.StartPos == nil) then
		self.StartPos = self:GetPos();

		local angles = self:GetAngles();
		--LogVec("Angles", angles);
		self.Acceleration = {x=0, y=0, z=-9.81};

		RotateVectorAroundR( self.Acceleration, self.Acceleration, g_Vectors.v100, angles.x );
		RotateVectorAroundR( self.Acceleration, self.Acceleration, g_Vectors.v010, angles.y );
		RotateVectorAroundR( self.Acceleration, self.Acceleration, g_Vectors.v001, angles.z );

		--LogVec("Acceleration", self.Acceleration);

		if (self.Acceleration.z > 0.0) then
			--Log("Acceleration is upwards! flipping it");
			self.Acceleration.x = -self.Acceleration.x;
			self.Acceleration.y = -self.Acceleration.y;
			self.Acceleration.z = -self.Acceleration.z;
			--LogVec("Acceleration", self.Acceleration);
		end

		local direction = {};
		CopyVector(direction, self.Acceleration);
		NormalizeVector(direction);

		self.DropPos = ScaleVector(direction, -self.START_FALL_HEIGHT);
		FastSumVectors(self.DropPos, self.DropPos, self.StartPos);

		--LogVec("StartPos", self.StartPos);
		--LogVec("DropPos", self.DropPos);
	end

	if (self.IsActive or System.IsEditor()) then
		self:ActivateCapturePoint(false);
	end

	local radius = self.Properties.ControlRadius;
	local offsetZ = self.Properties.ControlOffsetZ;
	local height = self.Properties.ControlHeight;

	local min = { x=-radius, y=-radius, z=offsetZ };
	local max = { x=radius, y=radius, z=(height + offsetZ) };

	self:SetTriggerBBox( min, max );

	if (System.IsEditor()) then
		if (self.Properties.DebugDraw ~= 0) then
			self:Activate(1);
		else
			self:Activate(0);
		end
	else
		self:Hide(1);
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:ShouldShowHudIcon()
	if (self.Falling) then
		local bottomZ = self.StartPos.z;
		local topZ = self.DropPos.z;
		local currentZ = self:GetPos().z;

		if (((currentZ - bottomZ) / (topZ - bottomZ)) > CrashSiteLocation.ICON_HEIGHT) then
			return false;
		end
	elseif (self.InFlowGraph == true) then
		return self.HasReceivedIconEvent;
	end
	return true;
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:ShouldDoPulseEffect()
	if (self.Falling or self.InFlowGraph) then
		return false;
	else
		return true;
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:Update(frameTime)
	if (self.Falling) then
		local dV = ScaleVector(self.Acceleration, frameTime);

		FastSumVectors(self.Velocity, self.Velocity, dV);
		
		local currentPos = self:GetPos();

		FastSumVectors(currentPos, currentPos, ScaleVector(self.Velocity, frameTime));

		if (currentPos.z < self.StartPos.z) then
			currentPos = self.StartPos;
			self.Falling = false;
			self:Event_Landed();
		end
		
		self:SetPos(currentPos);
	elseif (self.InDeathAnim == true) then
		self.TimeToDeath = self.TimeToDeath - frameTime;
		if (self.TimeToDeath < 0.0) then
			self:OnFinished(true);
			self.InDeathAnim = false;
			BroadcastEvent(self, "DeActivated");
		end
	end

	if (self.ColourChangePerc < 1.0) then
		self.ColourChangePerc = self.ColourChangePerc + frameTime;
		if (self.ColourChangePerc > 1.0) then
			self.ColourChangePerc = 1.0;
		end

		local invPerc = 1.0 - self.ColourChangePerc;
		local currentColour = {};
		currentColour.x = (self.PreviousColour.x * invPerc) + (self.TargetColour.x * self.ColourChangePerc);
		currentColour.y = (self.PreviousColour.y * invPerc) + (self.TargetColour.y * self.ColourChangePerc);
		currentColour.z = (self.PreviousColour.z * invPerc) + (self.TargetColour.z * self.ColourChangePerc);

		self:SetMaterialVec3(0, 1, "diffuse", currentColour);
	end

	if ((self.InFlowGraph == true) and (self.SmokeSlot == -1)) then
		if (self:IsHidden() == false) then
			self:StartSmoke();
		end
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation.Server:OnUpdate(frameTime)
	if (self.Properties.DebugDraw ~= 0) then
		local pos = self:GetWorldPos();
		local radius = self.Properties.ControlRadius;
		local height = self.Properties.ControlHeight;
		local offsetZ = self.Properties.ControlOffsetZ;

		pos.z = pos.z + (height * 0.5) + offsetZ;

		Game.DebugDrawCylinder(pos.x, pos.y, pos.z, radius, height, 60, 60, 255, 100);
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:ActivateCapturePoint(animate)
	--Log("CrashSiteLocation.ActivateCapturePoint(name=%s, animate=%s)", EntityName(self), tostring(animate));
	if ((self.InDeathAnim == true) or (self.IsActive == true)) then
		-- If we're already active, make sure we're cleaned up before activating again
		self.InDeathAnim = false;
		BroadcastEvent(self, "DeActivated");
	end
	self.IsActive = true;
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 0);

	if (animate) then
		if (self.Properties.UseFlowGraph == 0) then
			self.Velocity = {x=0, y=0, z=0};
			self:SetPos(self.DropPos);
			self.Falling = true;
			self:StartSmoke();
			self:Hide(0);
		else
			BroadcastEvent(self, "Activated");
			self.InFlowGraph = true;
			self.HasReceivedIconEvent = false;
		end
	else
		self:SetPos(self.StartPos);
		self:DoLandingFlowgraphEvent();
	end

	self:SetViewDistRatio(255);

	self:SetTargetColour(g_gameRules.game:GetTeam(self.id));
	
	--g_gameRules.game:ReRecordEntity(self.id);

	if (CryAction.IsServer()) then
		g_gameRules.game:ResetAlienTech(self.id);
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:StartSmoke()
	self.SmokeSlot = self:LoadParticleEffect(self.SmokeSlot, self.PARTICLE_EFFECT_TRAIL, {});
	--Log("Activated smoke, slot=%d", self.SmokeSlot);
	GameAudio.PlayEntitySignal(self.AUDIO_START, self.id);
	end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:IsAlertActive()
	if (self.AlertActive) then
		return 1;
	else
		return 0;
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:DeactivateCapturePoint(animate)
	--Log("CrashSiteLocation.DeactivateCapturePoint(name=%s, animate=%s)", EntityName(self), tostring(animate));
	self.IsActive = false;
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 2);

	if (self.Falling) then
		self.Falling = false;
		self.TimeToDeath = 0.0;
	end

	if (animate) then
		if (self.Falling) then
			self.Falling = false;
			self.TimeToDeath = 0.0;
		else
			self.SmokeSlot = self:LoadParticleEffect(self.SmokeSlot, self.PARTICLE_EFFECT_DEATH_ANIM, {});
			self.InDeathAnim = true;
			self.TimeToDeath = self.DEATH_ANIM_LENGTH;
			GameAudio.PlayEntitySignal(self.AUDIO_VOLATILE, self.id);

			local numChildren = self:GetChildCount();
			local i = 0;
			while i < numChildren do
				local childEnt = self:GetChild(i);
				if (childEnt ~= nil) then
					if (childEnt.bCurrentlyPickable == 1) then
						childEnt.bCurrentlyPickable = 0;
					end
				end
				i = i + 1;
			end
		end
	else
		self:OnFinished(animate);
		BroadcastEvent(self, "DeActivated");
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:OnFinished(animate)
	-- Make sure objects around get woken up
	local physFlags = { flags_mask=pef_always_notify_on_deletion, flags=pef_always_notify_on_deletion };
	self:SetPhysicParams( PHYSICPARAM_FLAGS, physFlags );
	
	self.AlertActive = false;
	self:ClearSmoke();
	self:Hide(1);
	
	--g_gameRules.game:ReRecordEntity(self.id);
	
	if (animate and not self.Falling) then
		Particle.SpawnEffect(self.PARTICLE_EFFECT_EXPLOSION, self:GetPos(), g_Vectors.v000);
		GameAudio.PlayEntitySignal(self.AUDIO_EXPLODE, self.id);
	end

  if (g_gameRules and CryAction.IsServer()) then
	 	local pos = self:GetPos();
	 	pos.z = pos.z + 1.5;
	 	local dir = {x=0.0, y=0.0, z=1.0};
	 	local exp = self.EXPLOSION_INFO_2;
	 	local hitType = g_gameRules.game:GetHitTypeId(exp.type);

	 	g_gameRules.game:ServerExplosion(NULL_ENTITY, self.id, exp.damage, pos, dir, exp.max_radius, 
	 																	0.0, exp.pressure, exp.hole_size, "", 0.0, hitType, exp.min_radius, 
	 																	exp.min_phys_radius, exp.max_phys_radius);
  end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:ClearSmoke()
	if (self.SmokeSlot ~= -1) then
		self:DeleteParticleEmitter(self.SmokeSlot);
		self:FreeSlot(self.SmokeSlot);
		self.SmokeSlot = -1;
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:OnSetTeam(teamId)
	self:SetTargetColour(teamId);
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:LocalPlayerChangedTeam()
	self:SetTargetColour(g_gameRules.game:GetTeam(self.id));
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:SetTargetColour(teamId)
	local localTeamId = -1;
	if (g_localActorId) then
		localTeamId = g_gameRules.game:GetTeam(g_localActorId);
	end

	CopyVector(self.PreviousColour, self.TargetColour);

	if (teamId == 0) then
		-- Neutral
		self.TargetColour.x = 0.3019;		-- 77
		self.TargetColour.y = 0.3019;		-- 77
		self.TargetColour.z = 0.3019;		-- 77
		--Log("CrashSiteLocation:SetTargetColour, setting to neutral, team=%i local=%i", teamId, localTeamId);
	elseif (teamId == localTeamId) then
		-- Blue
		self.TargetColour.x = 0.0784;		-- 20
		self.TargetColour.y = 0.2588;		-- 66
		self.TargetColour.z = 0.6784;		-- 173
		--Log("CrashSiteLocation:SetTargetColour, setting to blue, team=%i local=%i", teamId, localTeamId);
	else
		-- Red
		self.TargetColour.x = 0.7019;		-- 179
		self.TargetColour.y = 0.0705;		-- 18
		self.TargetColour.z = 0.0;			-- 0
		--Log("CrashSiteLocation:SetTargetColour, setting to red, team=%i local=%i", teamId, localTeamId);
	end
	self.ColourChangePerc = 0.0;
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:Event_Landed()
	GameAudio.PlayEntitySignal(self.AUDIO_LAND, self.id);
	GameAudio.Announce("Landed", 0); -- 0 context is CAnnouncer::eAC_inGame
	Particle.SpawnEffect(self.PARTICLE_EFFECT_CRASH, self:GetPos(), g_Vectors.v000);
	if (self.SmokeSlot ~= -1) then
		--Log("CrashSiteLocation.Update: deactivated smoke");
		self:DeleteParticleEmitter(self.SmokeSlot);
		self:FreeSlot(self.SmokeSlot);
		self.SmokeSlot = -1;
	end
	self.InFlowGraph = false;

	--g_gameRules.game:ReRecordEntity(self.id);

	if (g_gameRules and CryAction.IsServer()) then
		local pos = self:GetPos();
		pos.z = pos.z + 1.5;
		local dir = {x=0.0, y=0.0, z=1.0};
		local exp = self.EXPLOSION_INFO;
		local hitType = g_gameRules.game:GetHitTypeId(exp.type);

		g_gameRules.game:ServerExplosion(NULL_ENTITY, self.id, exp.damage, pos, dir, exp.max_radius, 
																		0.0, exp.pressure, exp.hole_size, "", 0.0, hitType, exp.min_radius, 
																		exp.min_phys_radius, exp.max_phys_radius);
	end

	self:DoLandingFlowgraphEvent();
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:Event_Show()
	--Log("CrashSiteLocation:Event_Show()");
	self:Hide(0);
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:DoLandingFlowgraphEvent()
	if (self.NumTimesLanded == 0) then
		BroadcastEvent(self, "OnFirstLanding");
	else
		BroadcastEvent(self, "OnRepeatLanding");
	end
	BroadcastEvent(self, "OnAnyLanding");
	self.NumTimesLanded = self.NumTimesLanded + 1;
	if (g_gameRules.game:GetTeam(self.id) == 0) then
		HUD.OnGameStatusUpdate(0, self.NEUTRAL_GAMESTATE_STRING);	-- eGBNFLP_Neutral
	end
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:Event_ShowIcon()
	self.HasReceivedIconEvent = true;
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:TestFlowGraph()		-- Called through the editor
	self:ActivateCapturePoint(true);
end

----------------------------------------------------------------------------------------------------
function CrashSiteLocation:TestPulseEffect()		-- Called through the editor
	Particle.SpawnEffect("cw2_gameplay.crash_site.drop_pod_capture_area", self:GetPos(), {x=0.0, y=0.0, z=1.0}, self.Properties.radiusEffectScale);
end

----------------------------------------------------------------------------------------------------
CrashSiteLocation.FlowEvents =
{
	Inputs =
	{
		Landed = { CrashSiteLocation.Event_Landed, "any" },
		ShowIcon = { CrashSiteLocation.Event_ShowIcon, "any" },
		Show = { CrashSiteLocation.Event_Show, "any" },
	},
	Outputs =
	{
		Activated = "bool",
		OnFirstLanding = "bool",
		OnRepeatLanding = "bool",
		OnAnyLanding = "bool",
		DeActivated = "bool",
	},
}

