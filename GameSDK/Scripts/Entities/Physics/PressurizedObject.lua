----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Pressurized Object Entity
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 19:5:2005   14:35 : Created by Márcio Martins
--
----------------------------------------------------------------------------------------------------
local SELF_NOSLOT = -1;  -- "constant" used in some functions to access the current object instead one of its slots
PressurizedObject =
{
	Properties =
	{
		bAutoGenAIHidePts = 0,
		objModel = "objects/props/misc/gas_canister/gascanister_con.cgf",
		Vulnerability =
		{
			bExplosion = 1,
			bCollision = 1,
			bMelee = 1,
			bBullet = 1,
			bOther = 1,
		},
		DamageMultipliers =
		{
			fCollision = 1.0,
			fBullet = 1.0,
		},
		fDamageTreshold = 0,
		Leak =
		{
			Effect =
			{
				ParticleEffect = "smoke_and_fire.geysir.intense_steam_small",
				PulsePeriod = 0,
				Scale = 1,
				CountScale = 1,
				bCountPerUnit = 0,
				bSizePerUnit = 0,
				AttachType = "none",
				AttachForm = "none",
				bPrime = 1,
			},
			Damage = 100,
			DamageRange = 3,
			DamageHitType = "fire",
			Pressure = 100,
			PressureDecay = 10,
			PressureImpulse = 10,
			MaxLeaks = 10,
			ImpulseScale = 1,
			Volume = 10;
			VolumeDecay = 1;
		},
		bPlayerOnly = 1,
		fDensity = -1,
		fMass = 25,
		bResting = 1, -- If rigid body is originally in resting state.
		bRigidBody = 1,
		bCanBreakOthers = 0,
		bPushableByPlayers  = 0,
		PhysicsBuoyancy =
		{
			water_density = 1,
			water_damping = 1.5,
			water_resistance = 0,
		},
		PhysicsSimulation =
		{
			max_time_step = 0.01,
			sleep_speed = 0.04,
			damping = 0,
		},
		TacticalInfo =
		{
			bScannable = 0,
			LookupName = "",
		},
	},
	Client = {};
	Server = {};
	Editor =
	{
		Icon = "tornado.bmp",
		IconOnTop = 1,
	},
};


Net.Expose{
	Class = PressurizedObject,
	ClientMethods = {
		AddLeak = { RELIABLE_ORDERED, PRE_ATTACH, VEC3, VEC3 },
	},
	ServerMethods = {
	},
	ServerProperties = {
	}
}


----------------------------------------------------------------------------------------------------
function PressurizedObject:IsUsable(user, idx)
	local ret = nil;
	if not self.__usable then self.__usable = self.Properties.bUsable end;

	local mp = System.IsMultiplayer();
	if(mp and mp~=0) then
		return 0;
	end

	if (self.__usable == 1) then
		ret = 2
	else
		if (user and user.CanGrabObject) then
			ret = user:CanGrabObject(self)
		end
	end

	return ret or 0
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:OnSpawn()
	local model = self.Properties.objModel;
	if (string.len(model) > 0) then
		local ext = string.lower(string.sub(model, -4));

		if ((ext == ".chr") or (ext == ".cdf") or (ext == ".cga")) then
			self:LoadCharacter(0, model);
		else
			self:LoadObject(0, model);
		end
	end
	g_gameRules.game:MakeMovementVisibleToAI("PressurizedObject");
end


----------------------------------------------------------------------------------------------------
function PressurizedObject.Server:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject.Client:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
	self:CacheResources();
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:CacheResources()
	self:PreLoadParticleEffect( self.Properties.Leak.Effect.ParticleEffect );
end
----------------------------------------------------------------------------------------------------
function PressurizedObject:OnReset()
	self.bScannable = self.Properties.TacticalInfo.bScannable;

	-- no problem in repeatedly registering or unregistering
	if (self.bScannable==1) then
		Game.AddTacticalEntity(self.id, eTacticalEntity_Story);
	else
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
	end

	local params =
	{
		mass = self.Properties.fMass,
		density = self.Properties.fDensity,
	};

	local bRigidBody = (tonumber(self.Properties.bRigidBody) ~= 0);
	if (CryAction.IsImmersivenessEnabled() == 0) then
		bRigidBody = nil
	end

	if (bRigidBody) then
		self:Physicalize(0, PE_RIGID, params);

		if (tonumber(self.Properties.bResting) ~= 0) then
			self:AwakePhysics(0);
		else
			self:AwakePhysics(1);
		end

		self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties.PhysicsBuoyancy);
		self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.Properties.PhysicsSimulation);
	else
		self:Physicalize(0, PE_STATIC, params);
	end

	local PhysFlags = {};
	PhysFlags.flags =  0;
	if (self.Properties.bPushableByPlayers == 1) then
		PhysFlags.flags = pef_pushable_by_players;
	end
	if (self.Properties.bCanBreakOthers==nil or self.Properties.bCanBreakOthers==0) then
		PhysFlags.flags = PhysFlags.flags+pef_never_break;
	end
	PhysFlags.flags_mask = pef_pushable_by_players + pef_never_break;
	self:SetPhysicParams( PHYSICPARAM_FLAGS,PhysFlags );

	self:Activate(0);
	self:ClearLeaks();

	self.pressure = self.Properties.Leak.Pressure;
	self.totalPressure=self.pressure;
	self.pressureDecay = self.Properties.Leak.PressureDecay;
	self.pressureImpulse = self.Properties.Leak.PressureImpulse;
	self.maxLeaks = self.Properties.Leak.MaxLeaks;

	self.damage = self.Properties.Leak.Damage;
	self.damageRange = self.Properties.Leak.DamageRange;

	self.damageCheckTime = 0.5;
	self.damageCheckTimer = self.damageCheckTime;

	self.shooterId = nil;
	self.volume=self.Properties.Leak.Volume;
	if (self.volume>0) then
		local physVolume = self:GetVolume(0);
		if (physVolume == 0) then
			physVolume = 1;
		end

		self.volumeConv = self.volume / physVolume; -- convert property volume to physics volume
		self.totalVolume = self.volume;
	end

	-- Mark AI hideable flag.
	if (self.Properties.bAutoGenAIHidePts == 1) then
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
	else
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
	end

end

----------------------------------------------------------------------------------------------------
function PressurizedObject:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:OnDestroy()
  if (self.bScannable==1) then
	  Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject.Server:OnHit(hit)
	if (hit.explosion or (not hit.normal)) then
		return;
	end

	local playerOnly = NumberToBool(self.Properties.bPlayerOnly);
	local playerHit = hit.shooterId == g_localActorId;

	local damage = hit.damage;
	local vul = self.Properties.Vulnerability;
	local mult = self.Properties.DamageMultipliers;

	local pass = true;
	if (hit.explosion) then pass = NumberToBool(vul.bExplosion);
	elseif (hit.type=="collision") then pass = NumberToBool(vul.bCollision); damage = damage * mult.fCollision;
	elseif (hit.type=="bullet") then pass = NumberToBool(vul.bBullet); hit.damage = damage * mult.fBullet;
	elseif (hit.type=="melee") then pass = NumberToBool(vul.bMelee);
	else pass = NumberToBool(vul.bOther); end

	pass = pass and damage >= self.Properties.fDamageTreshold;

	if(not pass)then return;end;

	if ((not hit.shooterId) or (not playerOnly) or playerHit) then
		self:Event_Hit();
	end

	if (self.leaks < self.maxLeaks and CryAction.IsImmersivenessEnabled() ~= 0) then
		local LPos = self:ToLocal( SELF_NOSLOT, hit.pos );
		local LDir = self:VectorToLocal( SELF_NOSLOT, hit.normal );

		self.allClients:AddLeak( LPos, LDir);  -- send values in local object space

		if (self.leaks==0) then
			self.shooterId=hit.shooterId;
		end
	end

	self:Activate(1);
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:CheckDamage(frameTime)
	self.damageCheckTimer = self.damageCheckTimer-frameTime;
	if (self.damageCheckTimer<= 0) then
		self.damageCheckTimer = self.damageCheckTime;
	else
		return;
	end

	if (self.leaks > 0) then
		for i,leak in ipairs(self.leakInfo) do
			self.leakPos = self:GetSlotWorldPos(leak.slot, self.leakPos);
			self.leakDir = self:GetSlotWorldDir(leak.slot, self.leakDir);

			local hits = Physics.RayWorldIntersection(self.leakPos, vecScale(self.leakDir, self.damageRange), 2, ent_all, self.id, nil, g_HitTable);
			if (hits > 0) then
				local entity = g_HitTable[1].entity;
				if (entity) then
					local dead = (entity.IsDead and entity:IsDead());
					if ((not dead) and entity.Server and entity.Server.OnHit) then
						local damage = (self.damage*self.damageCheckTime)/self.leaks;
						g_gameRules:CreateHit(entity.id,self.shooterId,self.id,damage,nil,nil,nil,self.Properties.DamageHitType);
					end
				end
			end
		end
	end
end

function PressurizedObject:Event_Hide()
	self:Hide(1);
end;

------------------------------------------------------------------------------------------------------
function PressurizedObject:Event_UnHide()
	self:Hide(0);
end;

----------------------------------------------------------------------------------------------------
function PressurizedObject:HasBeenScanned()
	self:ActivateOutput("Scanned", true);
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:UpdateLeaks(frameTime)

	if (self.volume<=0 and self.leaks>0) then
		self:ClearLeaks();
	end

	self.gravity=self:GetGravity(self.gravity);

	for i,v in ipairs(self.leakInfo) do
		self:UpdateLeak(frameTime, v, vecNormalize(vecScale(self.gravity, -1)));
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:UpdateLeak(frameTime, leak, gravityDir)

	self.leakPos = self:ToGlobal(SELF_NOSLOT, leak.pos);
	local submergedVolume=self:GetSubmergedVolume(0, gravityDir, self.leakPos)*self.volumeConv;

	local leaking=false;
	if (submergedVolume<self.volume) then
		leaking=true;
	end

	if (leaking or self.pressure>0) then
		self.volume=self.volume-self.Properties.Leak.VolumeDecay*frameTime;
		if (self.volume <=0) then
			self.volume=0;
		else
			self:StartLeaking(leak);
		end
	else
		self:StopLeaking(leak);
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:StartLeaking(leak)
	if (not leak.leaking) then
		leak.leaking=true;
		local WPos = self:ToGlobal(SELF_NOSLOT, leak.pos);
		local WDir = self:VectorToGlobal(SELF_NOSLOT, leak.dir);
		leak.slot = self:LoadParticleEffect(-1, self.Properties.Leak.Effect.ParticleEffect, self.Properties.Leak.Effect);
		self:SetSlotWorldTM(leak.slot, WPos, WDir );
		self.leaks = self.leaks+1;
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:StopLeaking(leak)
	if (leak.leaking) then
		leak.leaking=false;
		self:FreeSlot(leak.slot);
		leak.slot = -1;
		self.leaks = self.leaks-1;
	end
end

----------------------------------------------------------------------------------------------------
-- pos, dir, need to be in local object space
function PressurizedObject.Client:AddLeak(pos, dir)
	local leak = {};
	leak.pos = pos;
	leak.dir = dir;
	leak.slot = -1;
	leak.leaking=false;
	table.insert(self.leakInfo, leak);
	if (not self.IsActive()) then
		self:Activate(1);
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:ClearLeaks()
	if (self.leakInfo) then
		for i,v in ipairs(self.leakInfo) do
			self:StopLeaking(v);
			if (v.slot) then
				self:FreeSlot(v.slot);
			end
		end
	end

	self.leaks = 0;
	self.leakInfo = {};
end

----------------------------------------------------------------------------------------------------
function PressurizedObject.Server:OnUpdate(frameTime)
	self:CheckDamage(frameTime);
end

----------------------------------------------------------------------------------------------------
function PressurizedObject.Client:OnUpdate(frameTime)
	self:UpdateLeaks(frameTime);
	local decay = self.pressureDecay*self.leaks*frameTime;
	self.pressure = self.pressure-decay;
	if (self.pressure<0) then
		self.pressure = 0;
	end

	if (self.pressure>0 and self.leaks > 0) then
		local impulse = ((self.pressureImpulse*self.pressure)/self.leaks)*frameTime*self.Properties.Leak.ImpulseScale;

		if (impulse>0) then
			for i,leak in ipairs(self.leakInfo) do
				self.impulseDir = self:GetSlotWorldDir(leak.slot, self.impulseDir);
				self.impulsePos = self:GetSlotWorldPos(leak.slot, self.impulsePos);

				self:AddImpulse(-1, self.impulsePos, self.impulseDir, -impulse, 1);
			end
		end
	elseif (self.pressure<=0) then
		if (self.volume<=0) then
			self:ClearLeaks();
		if (self.bScannable==1) then
			Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
			end
		end
	end

	if (self.leaks < 1) then
		self:Activate(0);
	end
end

----------------------------------------------------------------------------------------------------
function PressurizedObject:Event_Hit(sender)
	BroadcastEvent(self, "Hit");
end

----------------------------------------------------------------------------------------------------
PressurizedObject.FlowEvents =
{
	Inputs =
	{
		Hit = { PressurizedObject.Event_Hit, "bool" },
		Hide = { PressurizedObject.Event_Hide, "bool" },
		UnHide = { PressurizedObject.Event_UnHide, "bool" },
	},
	Outputs =
	{
		Hit = "bool",
		Hide = "bool",
		UnHide = "bool",
		Scanned = "bool",
	},
}

MakeUsable(PressurizedObject);
MakePickable(PressurizedObject);
MakeTargetableByAI(PressurizedObject);
MakeKillable(PressurizedObject);
AddInteractLargeObjectProperty(PressurizedObject);