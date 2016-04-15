----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Destroyable Object Entity
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 16:8:2005   10:38 : Created by Márcio Martins
--
----------------------------------------------------------------------------------------------------
DestroyableObject =
{
	Client = {},
	Server = {},
	States = {"Alive","Dead" },

	Properties =
	{
		soclasses_SmartObjectClass = "",
		bAutoGenAIHidePts = 0,

		object_Model = "objects/props/industrial/barrels/barrel_a/barrel_a.cgf", -- Pre-destroyed model/submodel.
		ModelSubObject = "",
		object_ModelDestroyed = "objects/props/industrial/barrels/barrel_a/barrel_a_destroyed.cgf", -- Post-destroyed model/submodel (same as Model if blank).
		DestroyedSubObject = "",
		DmgFactorWhenCollidingAI = 1,

		bPlayerOnly = 1, -- Damaged only by player.
		bOnlyAllowPlayerToFullyDestroyObject = 0,
		fDamageTreshold = 0, -- Only accept damage higher than this value.
		bExplode = 1, -- Create explosion, using Explosion props

		Vulnerability =
		{
			bExplosion = 1,
			bCollision = 1,
			bMelee = 1,
			bBullet = 1,
			bOther = 1,
			projectileClass = "",
		},

		DamageMultipliers =
		{
			fCollision = 1.0,
			fBullet = 1.0,
			fProjectileClass = 1.0,
		},

		Breakage = -- => BreakToPieces
		{
			fLifeTime = 10, -- Average lifetime of particle pieces
			fExplodeImpulse = 0, -- Applies central impulse to pieces, in addition to hit impulse
			bSurfaceEffects = 1, -- Generate secondary particle effects from surface type
		},

		Explosion =
		{
			Delay = 0,
			ParticleEffect = "explosions.grenade_air.explosion",
			EffectScale = 1,
			MinRadius = 5,
			Radius = 10,
			MinPhysRadius = 2.5,
			PhysRadius = 5,
			SoundRadius = 0,
			Pressure = 1000,
			Damage = 1000,
			Direction = {x=0, y=0, z=1},
			vOffset = {x=0, y=0, z=0},
			DelayEffect =
			{
				bHasDelayEffect = 0,
				ParticleEffect = "",
				vOffset = {x=0, y=0, z=0},
				vRotation = {x=0, y=0, z=0},
				Params =
				{
					SpawnPeriod = 0,
					Scale = 1,
					CountScale = 1,
					bCountPerUnit = 0,
					bSizePerUnit = 0,
					AttachType = "none",
					AttachForm = "none",
					bPrime = 0,
				},
			},
		},

		Sounds =
		{
			sound_Alive = "",
			sound_Dead = "",
			sound_Dying = "",
			fAISoundRadius = 30,
			bStopSoundsOnDestroyed = 1,
		},

		Physics =  -- Particle pieces always physicalised as rigid bodies
		{
			bRigidBody = 1, -- True if rigid body.
			bRigidBodyActive = 1, -- If rigid body is originally created (1) OR will be created only on OnActivate (0).
			bRigidBodyAfterDeath = 1, -- True if rigid body after death too.
			bActivateOnDamage = 0, -- Activate when a rocket hit the entity.
			Density = -1,
			Mass = 100,
			bPushableByPlayers = 0,
			bCanBreakOthers = 0,
			Simulation =
			{
				max_time_step = 0.02,
				sleep_speed = 0.04,
				damping = 0,
			},
			MP =
			{
				bDontSyncPos=0,
			},
		},
		TacticalInfo =
		{
			bScannable = 0,
			LookupName = "",
		},
	},

	Editor =
		{
			Icon = "explosion.bmp",
			IsScalable = false;
		},
}

local Physics_DX9MP_Simple =  -- Particle pieces always physicalised as rigid bodies
{
	bRigidBody = 0, -- True if rigid body.
	bRigidBodyActive = 1, -- If rigid body is originally created (1) OR will be created only on OnActivate (0).
	bRigidBodyAfterDeath = 0, -- True if rigid body after death too.
	bActivateOnDamage = 0, -- Activate when a rocket hit the entity.
	Density = -1,
	Mass = -1,
}


Net.Expose {
	Class = DestroyableObject,
	ClientMethods = {
		ClActivateDelayEffect = { RELIABLE_ORDERED, POST_ATTACH, },
		ClExplode = { RELIABLE_ORDERED, POST_ATTACH, },
		ClUsedBy = { RELIABLE_ORDERED, NO_ATTACH, ENTITYID },
	},
	ServerMethods = {
		SvRequestUsedBy = { RELIABLE_UNORDERED, NO_ATTACH, ENTITYID },
	},
	ServerProperties = {
	},
};


-------------------------------------------------------
function DestroyableObject:OnLoad(table)

	self.bTemporaryUsable = table.bTemporaryUsable;
	self.shooterId = table.shooterId;
	self.health = table.health;
	self.exploded = table.exploded;
	self.rigidBodySlot = table.rigidBodySlot;
	self.isRigidBody = table.isRigidBody;
	self.currentSlot = table.currentSlot;
	self.LastHit = table.LastHit;

	if( (table.FXSlot or 0) <=0 and (self.FXSlot or 0) > 0) then
	  self:DeleteParticleEmitter( self.FXSlot );
		self:RemoveEffect();
	end
	self.FXSlot = table.FXSlot;

	self.dead = table.dead;

	self:SetCurrentSlot(self.currentSlot);


	if (self.dead) then
		if (self.Properties.Physics.bRigidBodyAfterDeath == 1) then
			-- temporarily set bRigidBody to 1, because EntityCommon checks it!
			local aux = self.Properties.Physics.bRigidBody;
			self.Properties.Physics.bRigidBody = 1;
			self:PhysicalizeThis(self.currentSlot);
			self.Properties.Physics.bRigidBody = aux;
		end
	else
		self:PhysicalizeThis(self.currentSlot);
	end

	if (self:GetState() ~= table.state) then
	  self:GotoState(table.state)
	end
end

-------------------------------------------------------
function DestroyableObject:OnSave(table)
	table.bTemporaryUsable = self.bTemporaryUsable;
	table.shooterId = self.shooterId;
	table.health = self.health;
	table.FXSlot = self.FXSlot;
	table.dead = self.dead;
	table.exploded = self.exploded;
	table.rigidBodySlot = self.rigidBodySlot;
	table.isRigidBody = self.isRigidBody;
	table.currentSlot = self.currentSlot;
	table.LastHit = self.LastHit;
	table.state = self:GetState();
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:CommonInit()
	self.bReloadGeoms = 1;
	self.bTemporaryUsable=0;
	if (not self.bInitialized) then
		self.LastHit = {
			impulse = {x=0,y=0,z=0},
			pos = {x=0,y=0,z=0},
		};
		self:Reload();
		self.bInitialized = 1;
		self:GotoState( "Alive" );
	end
	g_gameRules.game:MakeMovementVisibleToAI("DestroyableObject");
end

----------------------------------------------------------------------------------------------------
function DestroyableObject.Server:OnInit()
	self:CommonInit();
end

----------------------------------------------------------------------------------------------------
function DestroyableObject.Client:OnInit()
	self:CommonInit();
	self:CacheResources();
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:CacheResources()
	self:PreLoadParticleEffect( self.Properties.Explosion.ParticleEffect );
	self:PreLoadParticleEffect( self.Properties.Explosion.DelayEffect.ParticleEffect );
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:OnPropertyChange()
	self.bReloadGeoms = 1;
	self:Reload();
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:OnShutDown()
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:OnReset()
	self:RemoveEffect();

	if(self.timerShooterId)then
		--Log("self.timerShooterId: "..self.timerShooterId);
	end;

	self:ResetTacticalInfo();

	if (self:GetState() ~= "Alive") then
		self:Reload();
	end

	self:AwakePhysics(0);
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:RemoveEffect()
	if(self.FXSlot)then
		self:FreeSlot(self.FXSlot);
		self.FXSlot= -1;
	end;
end;

----------------------------------------------------------------------------------------------------
function DestroyableObject:ResetTacticalInfo()
	self.bScannable = self.Properties.TacticalInfo.bScannable;

	-- no problem in repeatedly registering or unregistering
	if (self.bScannable==1) then
		Game.AddTacticalEntity(self.id, eTacticalEntity_Hazard);
	else
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_Hazard);
	end
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:Reload()
  self:ResetTacticalInfo();
	self:ResetOnUsed();
	local props = self.Properties;
	self.bTemporaryUsable=self.Properties.bUsable;
	self.shooterId = NULL_ENTITY;
	self.health = props.Health.MaxHealth;
	self.dead = nil;
	self.exploded = nil;
	self.rigidBodySlot = nil;
	self.isRigidBody = nil;

	if (self.bReloadGeoms == 1) then
		if (not EmptyString(props.object_Model)) then
			self:LoadObject(3,props.object_Model); -- First load whole object in slot 3.
			self:DrawSlot(3,0); -- Make it invisible
			self:LoadSubObject(0,props.object_Model, props.ModelSubObject);
		end

		if (not EmptyString(props.object_ModelDestroyed)) then
			self:LoadSubObject(1, props.object_ModelDestroyed, props.DestroyedSubObject);
		elseif (not EmptyString(props.DestroyedSubObject)) then
			self:LoadSubObject(1, props.object_Model, props.DestroyedSubObject);
		end

		self:SetCurrentSlot(0);
		self:PhysicalizeThis(0);
	end

	-- stop old sounds
	-- REINSTANTIATE!!!
	-- self:StopAllSounds();
	self.bReloadGeoms = 0;
	self:GotoState( "Alive" );

		-- Mark AI hideable flag.
	if (props.bAutoGenAIHidePts == 1) then
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
	else
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
	end

	local wmin, wmax = self:GetWorldBBox(g_Vectors.temp_v1, g_Vectors.temp_v2)
	self.radius = 0.15 + math.max(wmax.x - wmin.x, math.max(wmax.y - wmin.y, wmax.z - wmin.z))
	self.center = { x = (wmax.x + wmin.x) * 0.5, y = (wmax.y + wmin.y) * 0.5, z = (wmax.z + wmin.z) * 0.5 };

end

------------------------------------------------------------------------------------------------------
function DestroyableObject:PhysicalizeThis( nSlot )

	if (self.Properties.Physics.MP.bDontSyncPos==1) then
		CryAction.DontSyncPhysics(self.id);
	end

	local Physics = self.Properties.Physics;
	if (CryAction.IsImmersivenessEnabled() == 0) then
		Physics = Physics_DX9MP_Simple;
	end
	-- Init physics.
	EntityCommon.PhysicalizeRigid( self,nSlot,Physics,self.bRigidBodyActive );
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:SetCurrentSlot(slot)
	if (slot == 0) then
		self:DrawSlot(0, 1);
		self:DrawSlot(1, 0);
	else
		self:DrawSlot(0, 0);
		self:DrawSlot(1, 1);
	end
	self.currentSlot = slot;
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:Explode()

	if (CryAction.IsImmersivenessEnabled() == 0) then
		return;
	end

	if( not CryAction.IsClient() ) then
		-- it will be skipped for server client in ClExplode
		self.allClients:ClExplode();
	end

	local Properties = self.Properties;
	self.bTemporaryUsable=0;
	self.bReloadGeoms = 1;

	local hitPos = self.LastHit.pos;
	local hitImp = self.LastHit.impulse;

	if( CryAction.IsClient() ) then
		self:BreakToPieces(
			0, 0,
			Properties.Breakage.fExplodeImpulse,
			hitPos,
				hitImp,
			Properties.Breakage.fLifeTime,
			Properties.Breakage.bSurfaceEffects
		);
	end

	AI.BreakEvent(self.id, self.center, self.radius);

	self:RemoveDecals();
	local bDeleteEntity = false;

	self:SetCurrentSlot(1);

	if (Properties.object_ModelDestroyed ~="" or Properties.DestroyedSubObject ~="") then
		if (Properties.Physics.bRigidBodyAfterDeath == 1) then
			-- temporarily set bRigidBody to 1, because EntityCommon checks it!
			local aux = Properties.Physics.bRigidBody;
			Properties.Physics.bRigidBody = 1;
			self:PhysicalizeThis(1);
			Properties.Physics.bRigidBody = aux;
			self:AwakePhysics(1);
		else
			self:PhysicalizeThis(1);
			self:AwakePhysics(1);
		end
	else
		-- if no destroyed model, this entity must be killed.
		bDeleteEntity = true;
	end


	if( CryAction.IsServer() and NumberToBool(self.Properties.bExplode)) then
		local expl = self.Properties.Explosion;

		local pos = self:GetWorldPos(g_Vectors.temp_v1);
		local dirX = self:GetDirectionVector(0, g_Vectors.temp_v2);
		local dirY = self:GetDirectionVector(1, g_Vectors.temp_v3);
		local dirZ = self:GetDirectionVector(2, g_Vectors.temp_v4);
		local offset = g_Vectors.temp_v5;
		CopyVector(offset, expl.vOffset);

		local explo_dir = g_Vectors.temp_v6;
		CopyVector(explo_dir, expl.Direction);

		pos.x = pos.x + dirX.x * offset.x + dirY.x * offset.y + dirZ.x * offset.z;
		pos.y = pos.y + dirX.y * offset.x + dirY.y * offset.y + dirZ.y * offset.z;
		pos.z = pos.z + dirX.z * offset.x + dirY.z * offset.y + dirZ.z * offset.z;
		local explo_pos = pos;

		local explosionType = g_gameRules.game:GetHitTypeId("explosion");

		g_gameRules:CreateExplosion(self.shooterId, self.id, expl.Damage, explo_pos, explo_dir, expl.Radius, nil, expl.Pressure, expl.HoleSize, expl.ParticleEffect, expl.EffectScale, expl.MinRadius, expl.MinPhysRadius, expl.PhysRadius, explosionType, expl.SoundRadius);
	end

	-- play the dead sound after explosion
	if (self.dead ~= true) then
		-- REINSTANTIATE!!!
		-- self:PlaySoundEvent(self.Properties.Sounds.sound_Dying,g_Vectors.v000,g_Vectors.v001,0,0,SOUND_SEMANTIC_MECHANIC_ENTITY);
	end
	-- REINSTANTIATE!!!
	-- self:PlaySoundEvent(self.Properties.Sounds.sound_Dead,g_Vectors.v000,g_Vectors.v001,0,0,SOUND_SEMANTIC_MECHANIC_ENTITY);

	self.exploded = true;

	BroadcastEvent( self,"Explode" );

	-- Must be in the last line.
	if (bDeleteEntity == true) then
		--self:DeleteThis();
		self:Hide(1);
	end
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:ActivateDelayEffect()
	local explosion=self.Properties.Explosion;
	if(explosion.DelayEffect.bHasDelayEffect==1)then
		if(not self.FXSlot or self.FXSlot==(-1))then
			if(not EmptyString(explosion.DelayEffect.ParticleEffect))then
				self.FXSlot=self:LoadParticleEffect( -1,explosion.DelayEffect.ParticleEffect,explosion.DelayEffect.Params);
				if (self.FXSlot) then
					self:SetSlotPos(self.FXSlot,explosion.DelayEffect.vOffset);
					self:SetSlotAngles(self.FXSlot,explosion.DelayEffect.vRotation);
				end
			end;
		end;
	end;
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:Die(hit)
	if (hit) then
		self.shooterId = hit.shooterId;
	else
		self.shooterId = NULL_ENTITY;
	end

	self.dead = true;
	if (self.health > 0) then
		self.health = 0;
	end
	-- REINSTANTIATE!!!
	-- self:PlaySoundEvent(self.Properties.Sounds.sound_Dying,g_Vectors.v000,g_Vectors.v001,0,0,SOUND_SEMANTIC_MECHANIC_ENTITY);

	-- if we didn't explode yet
	if (not self.exploded) then
		local explosion=self.Properties.Explosion;
		if(explosion.Delay>0 and not explosion.DelayEffect.bHasDelayEffect==1)then
			self:SetTimer(0,explosion.Delay*1000);
		else
			self:GotoState("Dead");
			if (hit and hit.dir) then
				self:AddImpulse(hit.partId or -1, hit.pos, hit.dir, hit.damage);
			end
		end
	end
end


----------------------------------------------------------------------------------------------------
function DestroyableObject.Server:OnHit(hit)
	CopyVector(self.LastHit.pos, hit.pos);
	CopyVector(self.LastHit.impulse, hit.dir or g_Vectors.v000);
	self.LastHit.impulse.x = self.LastHit.impulse.x * hit.damage;
	self.LastHit.impulse.y = self.LastHit.impulse.y * hit.damage;
	self.LastHit.impulse.z = self.LastHit.impulse.z * hit.damage;

	self:ActivateOutput("HitBy", hit.shooterId);

	if (self:IsInvulnerable()) then
		return true;
	end

	local damage = hit.damage;
	local vul = self.Properties.Vulnerability;
	local mult = self.Properties.DamageMultipliers;

	self.shooterId = hit.shooterId;

	local pass = true;
	if ((vul.projectileClass ~= "") and (vul.projectileClass == hit.projectileClass)) then
		pass = true;
		damage = damage * mult.fProjectileClass;
	elseif (hit.explosion) then
		pass = NumberToBool(vul.bExplosion);
	elseif (hit.type=="collision") then
		pass = NumberToBool(vul.bCollision);
		damage = damage * mult.fCollision;
	elseif (hit.type=="bullet") then
		pass = NumberToBool(vul.bBullet);
		damage = damage * mult.fBullet;
	elseif (hit.type=="melee") then
		pass = NumberToBool(vul.bMelee);
	else
		pass = NumberToBool(vul.bOther);
	end

	pass = pass and damage > self.Properties.fDamageTreshold;	-- damage needs to be higher than treshold
	--Log("%s != %s", tostring(hit.shooterId), tostring(g_localActorId));

	if ( (not System.IsMultiplayer()) and pass and NumberToBool(self.Properties.bPlayerOnly) and
		  (hit.shooterId and (hit.shooterId ~= g_localActorId))) then -- damage must come from player
		pass=false;
	end

	if (pass) then
		self.health = self.health - damage;

		-- In single player only the player is allowed to fully destroy the object
		if (NumberToBool(self.Properties.bOnlyAllowPlayerToFullyDestroyObject)) then
			if (not System.IsMultiplayer()) then
				if (hit.shooterId ~= g_localActorId) then
					if (self.health <= 0) then
						self.health = 1
					end
				end
			end
		end

		if (self.health <= 0) then
			self:Die(hit);
		end

		if (NumberToBool(self.Properties.bActivateOnDamage)) then
			self:AwakePhysics(1);
		end
		local explosion=self.Properties.Explosion;
		if(explosion.DelayEffect.bHasDelayEffect==1)then
			if(not self.FXSlot or self.FXSlot==(-1))then

				CopyVector(g_SignalData.point, self:GetPos())
				AI.FreeSignal(10, "OnExposedToExplosion", self:GetPos(), self.Properties.Explosion.Radius, self.id, g_SignalData)

				local rnd=randomF(0,1.5)
				--Log("Setting Delay on "..self:GetName()..": "..explosion.Delay+rnd);
				self:SetTimer(0,(explosion.Delay+rnd)*1000);
				if( CryAction.IsClient() ) then
					self:ActivateDelayEffect();
				end
				if( CryAction.IsServer() ) then
					-- it will be skipped for server client in ClActivateDelayEffect
					self.allClients:ClActivateDelayEffect();
				end
			end
		end
	end

	return true;
end

function DestroyableObject:CanBeMadeTargetable()
	return (self:GetState() == "Alive");
end

----------------------------------------------------------------------------------------------------
function DestroyableObject.Server:OnTimer(timerId, msec)
	if (timerId == 0) then
		self:GotoState("Dead");
	end
end

----------------------------------------------------------------------------------------------------
-- Alive State
----------------------------------------------------------------------------------------------------
DestroyableObject.Client.Alive =
{
	OnBeginState=function(self)
		-- REINSTANTIATE!!!
		-- self:PlaySoundEvent(self.Properties.Sounds.sound_Alive,g_Vectors.v000,g_Vectors.v001,0,0,SOUND_SEMANTIC_MECHANIC_ENTITY);
	end,

	OnPhysicsBreak = function( self,vPos,nPartId,nOtherPartId )
		self:DeactivateTacticalInfo()
	end,
}
DestroyableObject.Server.Alive =
{
	OnTimer = function(self,timerId,msec)
		if (timerId == 0) then
			self:GotoState( "Dead" );
		end
	end,
}

----------------------------------------------------------------------------------------------------
-- Dead State
----------------------------------------------------------------------------------------------------
DestroyableObject.Client.Dead =
{
	OnBeginState=function(self)
	self:DeactivateTacticalInfo()
		if(self.Properties.Sounds.bStopSoundsOnDestroyed == 1) then
			-- REINSTANTIATE!!!
			-- self:StopAllSounds();
		end
		if (not CryAction.IsServer()) then
			self:RemoveEffect();
			self:Explode();
			self.dead = true;
		end
	end,
}
DestroyableObject.Server.Dead =
{
	OnBeginState=function(self)
		self:RemoveEffect();
		self:Explode();
		self.dead = true;
	end,
}


----------------------------------------------------------------------------------------------------
function DestroyableObject:DeactivateTacticalInfo()
	if (self.bScannable==1) then
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
	end
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:Event_Reset( sender )
	self:OnReset();
	BroadcastEvent( self,"Reset" );
end

----------------------------------------------------------------------------------------------------
function DestroyableObject:Event_Explode( sender )
	if self:GetState()=="Dead" then return end
	if self.exploded then return end

	BroadcastEvent( self,"Explode" );
	BroadcastEvent( self, "Break" )

	self:Die();
end

function DestroyableObject:Base_OnUsed(user, idx)
	if( CryAction.IsServer() ) then
		self:ActivateOutput( "UsedBy", user.id );
		self.allClients:ClUsedBy(user.id);
	else
		self.server:SvRequestUsedBy(user.id);
	end
end

------------------------------------------------------------------------------------------------------
function DestroyableObject.Client:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
	self:DeactivateTacticalInfo();
	self:ActivateOutput("Break",nPartId+1 );
end


----------------------------------------------------------------------------------------------------
function DestroyableObject:IsUsable(user)

	local ret = nil
	if (self.Properties.bUsable == 1 and self.bTemporaryUsable == 1) then
		ret = 2
	else
		local PhysProps = self.Properties.Physics;
		if (PhysProps.bRigidBody == 1 and PhysProps.bRigidBodyActive == 1 and user.CanGrabObject) then
			ret = user:CanGrabObject(self);
		end
	end

	return ret or 0
end

function DestroyableObject:GetUsableMessage(idx)
	if (self.Properties.bUsable == 1 and self.bTemporaryUsable == 1) then
		return self.Properties.UseMessage;
	else
		return "@grab_object";
	end;
end

------------------------------------------------------------------------------------------------------
function DestroyableObject:Event_Hide()
	self:Hide(1);
	BroadcastEvent( self, "Hide" );
	--self:DrawObject(0,0);
	--self:DestroyPhysics();
end

------------------------------------------------------------------------------------------------------
function DestroyableObject:Event_UnHide()
	self:Hide(0);
	BroadcastEvent( self, "UnHide" );
	--self:DrawObject(0,1);
	--self:SetPhysicsProperties( 1,self.bRigidBodyActive );
end

DestroyableObject.FlowEvents =
{
	Inputs =
	{
		Explode = { DestroyableObject.Event_Explode, "bool" },
		Reset = { DestroyableObject.Event_Reset, "bool" },
		Used = { DestroyableObject.Event_Used, "bool" },
		EnableUsable = { DestroyableObject.Event_EnableUsable, "bool" },
		DisableUsable = { DestroyableObject.Event_DisableUsable, "bool" },
		Hide = { DestroyableObject.Event_Hide, "bool" },
		UnHide = { DestroyableObject.Event_UnHide, "bool" },
	},
	Outputs =
	{
		Explode = "bool",
		Reset = "bool",
		Used = "bool",
		EnableUsable = "bool",
		DisableUsable = "bool",
		Hide = "bool",
		UnHide = "bool",
		Break = "int",
		Scanned = "bool",
		HitBy = "entity",
		UsedBy = "entity"
	},
}

----------------------------------------------------------------------------------------------------
function DestroyableObject:HasBeenScanned()
	self:ActivateOutput("Scanned", true);
end

------------------------------------------------------------------------------------------------------
function DestroyableObject:SavePhysicalState()
	self.initPos = self:GetPos();
	self.initRot = self:GetWorldAngles();
	self.initScale = self:GetScale();
end

------------------------------------------------------------------------------------------------------
function DestroyableObject:RestorePhysicalState()
	if( not self.initPos ) then
		return;
	end

	self:KillTimer(0);

	if( self:IsHidden() ) then
		self:Hide(0);
	end

	self:RemoveEffect();
	self.bReloadGeoms = 1;
	self:Reload();
	self:AwakePhysics(0);

	self:SetPos(self.initPos);
	self:SetWorldAngles(self.initRot);
	self:SetScale(self.initScale);

end

--server functions
function DestroyableObject.Server:SvRequestUsedBy(userId)
	self:ActivateOutput( "UsedBy", userId )
	self.allClients:ClUsedBy(userId);
end

-- client functions
function DestroyableObject.Client:ClActivateDelayEffect()
	if( not CryAction.IsServer() ) then
		self:ActivateDelayEffect();
	end
end
-- client functions
function DestroyableObject.Client:ClExplode()
	if( not CryAction.IsServer() ) then
		self:Explode();
	end
end

function DestroyableObject.Client:ClUsedBy(userId)
	if( not CryAction.IsServer() ) then
		self:ActivateOutput( "UsedBy", userId )
	end
end

MakeUsable(DestroyableObject);
MakePickable(DestroyableObject);
MakeTargetableByAI(DestroyableObject);
MakeKillable(DestroyableObject);
AddInteractLargeObjectProperty(DestroyableObject);
MakeThrownObjectTargetable(DestroyableObject);