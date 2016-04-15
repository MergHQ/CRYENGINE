Script.ReloadScript( "Scripts/Default/Entities/Others/BasicEntity.lua" );

BreakableObject={
	Properties = {
		soclasses_SmartObjectClass = "",
		bAutoGenAIHidePts = 0,
		object_Model="", -- Default must be empty.
		nDamage=10,
		fBreakImpuls = 7,	-- impuls applayed when the object is broken
		nExplosionRadius=5,
		bExplosion=1,
		bTriggeredOnlyByExplosion=0,
		impulsivePressure=200, -- physics params
		rmin = 2.0, -- physics params
		rmax = 20.5, -- physics params
		Parts = {
			bRigidBody = 0,
			LifeTime = 20,
			Density = 100,
		},
		DyingSound={
			sndFilename="",
			InnerRadius=1,
			OuterRadius=10,
			nVolume=255,
		},
		Physics = {
			bRigidBody=0, -- True if rigid body.
			bRigidBodyActive = 1, -- If rigid body is originally created (1) OR will be created only on OnActivate (0).
			bActivateOnDamage = 0, -- Activate when a rocket hit the entity.
			bResting = 0, -- If rigid body is originally in resting state.
			Density = -1,
			Mass = 700,
			vector_Impulse = {x=0,y=0,z=0}, -- Impulse to apply at event.
			max_time_step = 0.01,
			sleep_speed = 0.04,
			damping = 0,
			water_damping = 0,
			water_resistance = 1000,	
			water_density = 1000,
			Type="Unknown",
			bFixedDamping = 0,
			HitDamageScale = 0,
		},
	},
	
	bBreakByCar=1,
	
	dead = 0,
	deathVector = {x=0,y=0,z=1 },
	
	isLoading = nil,
	
	--PHYSICS PARAMS
--	explosion_params = {
--		pos = nil,
--		damage = 100,
--		rmin = 2.0,
--		rmax = 20.5,
--		radius = 3,
--		impulsive_pressure = 200,
--		shooter = nil,
--		weapon = nil,
--	},

	-----------------------------------------------------------------------------
	-- Reassign physics related methods to basic entity.
	-----------------------------------------------------------------------------
	StopLastPhysicsSounds = BasicEntity.StopLastPhysicsSounds,
	OnStopRollSlideContact = BasicEntity.OnStopRollSlideContact,
}

function BreakableObject:OnInit()

	self:RegisterState("Active");
	self:RegisterState("Dead");

	self:Activate(0);
	self:TrackColliders(1);
	self.dead = 0;
	self:OnReset();
end

function BreakableObject:OnPropertyChange()
	self:OnReset();
end

function BreakableObject:OnShutDown()

end

------------------------------------------------------------------------------------------------------
-- Set Physics parameters.
------------------------------------------------------------------------------------------------------
function BreakableObject:SetPhysicsProperties()
	if (self.Properties.Physics.bRigidBody ~= 1) then
		return;
	end

  local Mass,Density,qual;
  
 	Mass    = self.Properties.Physics.Mass; 
 	Density = self.Properties.Physics.Density;
 	
  
		-- Make Rigid body.
	self:CreateRigidBody( Density,Mass,-1 );
	if (self.bCharacter == 1) then
		self:PhysicalizeCharacter( Mass,0,0,0);
	end

		local SimParams = {
			max_time_step = self.Properties.Physics.max_time_step,
			sleep_speed = self.Properties.Physics.sleep_speed,
			damping = self.Properties.Physics.damping,
			water_damping = self.Properties.Physics.water_damping,
			water_resistance = self.Properties.Physics.water_resistance,
			water_density = self.Properties.Physics.water_density,
			mass = Mass,
			density = Density
		};
	
	
		self.bRigidBodyActive = self.Properties.Physics.bRigidBodyActive;
		if (self.bRigidBodyActive ~= 1) then
			-- If rigid body should not be activated, it must have 0 mass.
			SimParams.mass = 0;
			SimParams.density = 0;
		end

		local Flags = { flags_mask=pef_fixed_damping, flags=0 };
		if (self.Properties.Physics.bFixedDamping~=0) then
			Flags.flags = pef_fixed_damping;
		end
		
		self:SetPhysicParams(PHYSICPARAM_SIMULATION, SimParams );
		self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties.Physics );
		self:SetPhysicParams(PHYSICPARAM_FLAGS, Flags );
		
		if (self.Properties.Physics.bResting == 0) then
			self:Activate(1);
			self:AwakePhysics(1);
		else
			self:Activate(0);
			self:AwakePhysics(0);
		end
		
	
	-- physics-sounds
	if (PhysicsSoundsTable) then
		local SoundDesc;
		-- load soft contact sounds
		self.ContactSounds_Soft={};
		if (PhysicsSoundsTable.Soft and PhysicsSoundsTable.Soft[self.Properties.Physics.Type]) then
			SoundDesc=PhysicsSoundsTable.Soft[self.Properties.Physics.Type].Impact;
			if (SoundDesc) then
				self.ContactSounds_Soft.Impact=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Soft.ImpactVolume=SoundDesc[3];
			end
			SoundDesc=PhysicsSoundsTable.Soft[self.Properties.Physics.Type].Roll;
			if (SoundDesc) then
				self.ContactSounds_Soft.Roll=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Soft.RollVolume=SoundDesc[3];
				Sound.SetSoundLoop(self.ContactSounds_Soft.Roll, 1);
			end
			SoundDesc=PhysicsSoundsTable.Soft[self.Properties.Physics.Type].Slide;
			if (SoundDesc) then
				self.ContactSounds_Soft.Slide=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Soft.SlideVolume=SoundDesc[3];
				Sound.SetSoundLoop(self.ContactSounds_Soft.Slide, 1);
			end
		else
			--System.Log("[BasicEntity] Warning: Table *PhysicsSoundsTable.Soft* or *PhysicsSoundsTable.Soft["..self.Properties.Physics.Type.."]* no found !");
		end
		-- load hard contact sounds
		self.ContactSounds_Hard={};
		if (PhysicsSoundsTable.Hard and PhysicsSoundsTable.Hard[self.Properties.Physics.Type]) then
			SoundDesc=PhysicsSoundsTable.Hard[self.Properties.Physics.Type].Impact;
			if (SoundDesc) then
				self.ContactSounds_Hard.Impact=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Hard.ImpactVolume=SoundDesc[3];
			end
			SoundDesc=PhysicsSoundsTable.Hard[self.Properties.Physics.Type].Roll;
			if (SoundDesc) then
				self.ContactSounds_Hard.Roll=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Hard.RollVolume=SoundDesc[3];
				Sound.SetSoundLoop(self.ContactSounds_Hard.Roll, 1);
			end
			SoundDesc=PhysicsSoundsTable.Hard[self.Properties.Physics.Type].Slide;
			if (SoundDesc) then
				self.ContactSounds_Hard.Slide=Sound.Load3DSound(SoundDesc[1], SoundDesc[2], SoundDesc[6], 255, SoundDesc[4], SoundDesc[5]);
				self.ContactSounds_Hard.SlideVolume=SoundDesc[3];
				Sound.SetSoundLoop(self.ContactSounds_Hard.Slide, 1);
			end
		else
			--System.Log("[BasicEntity] Warning: Table *PhysicsSoundsTable.Hard* or *PhysicsSoundsTable.Hard["..self.Properties.Physics.Type.."]* no found !");
		end
	else
		--System.Log("[BasicEntity] Warning: Table *PhysicsSoundsTable* no found !");
	end
end

function BreakableObject:Event_Die( sender )
	self:GoDead();
	BroadcastEvent(self, "Die");
end

function BreakableObject:Event_IsDead( sender )
	BroadcastEvent(self, "IsDead");
end



function BreakableObject:OnReset()
	self:Activate(0);
	self:TrackColliders(1);
	self.dead = 0;
	
	if (self.Properties.object_Model == "") then return end;
	
	if (self.Properties.object_Model ~= self.CurrModel) then
		self.CurrModel = self.Properties.object_Model;
		self:LoadBreakable(self.Properties.object_Model);
		self:CreateStaticEntity( 10,-2 );
	end
	
	-- stop old sounds
	if (self.DyingSound and Sound.IsPlaying(self.DyingSound)) then
		Sound.StopSound(self.DyingSound);
	end
	-- load sounds
	local SndTbl;
	SndTbl=self.Properties.DyingSound;
	if (SndTbl.sndFilename~="") then
		self.DyingSound=Sound.Load3DSound(SndTbl.sndFilename, SOUND_DEFAULT_3D);
		if (self.DyingSound) then
			Sound.SetSoundPosition(self.DyingSound, self:GetPos());
			Sound.SetSoundVolume(self.DyingSound, SndTbl.nVolume);
			Sound.SetMinMaxDistance(self.DyingSound, SndTbl.InnerRadius, SndTbl.OuterRadius);
		end
	else
		self.DyingSound=nil;
	end

	self.curr_damage=self.Properties.nDamage;

--	self.explosion_params.impulsive_pressure=self.Properties.impulsivePressure;	
--	self.curr_damage=self.Properties.nDamage;
--	self.explosion_params.radius=self.Properties.nExplosionRadius;
--	self.explosion_params.rmin=self.Properties.rmin;
--	self.explosion_params.rmax=self.Properties.rmax;
--
--	if(self.Properties.bExplosion==1)then
--		self.explosion=1;
--		if(self.Properties.ExplosionTable~="")then
--			self.explosion_table=globals()[self.Properties.ExplosionTable];
--		else
--			self.explosion_table=Materials.mat_default.projectile_hit;
--		end
--	end
	--System.Log("---RESET DESTROY");

	if(self.dead == 0) then
		self:GoAlive();
		
		-- Set physics.
		--DestroyableObject:SetPhysicsProperties( self );
		self:SetPhysicsProperties();
	else
		self:GoDead();
	end

	-- Mark AI hideable flag.
	if (self.Properties.bAutoGenAIHidePts == 1) then
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 0); -- set
	else
		self:SetFlags(ENTITY_FLAG_AI_HIDEABLE, 2); -- remove
	end

end

function BreakableObject:GoAlive()

	self:EnablePhysics(1);
	self:DrawObject(0,1);
	self:DrawObject(1,0);
	self:GotoState( "Active" );
end

function BreakableObject:GoDead( deathVector )
	if (deathVector) then
		self.deathVector = new(deathVector);
	else
		self.deathVector = {x=0,y=0,z=1};
	end
	if (self.DyingSound and (not Sound.IsPlaying(self.DyingSound))) then
		Sound.PlaySound(self.DyingSound);
		--System.Log("starting dying");
	end

	self:GotoState( "Dead" );
--	if(deathVector) then
--		self:BreakEntity(deathVector, self.Properties.fBreakImpuls,self.Properties.Parts.bRigidBody );
--	else	
--		self:BreakEntity({x=0,y=0,z=1}, self.Properties.fBreakImpuls,self.Properties.Parts.bRigidBody );
--	end	
--
--	self:DrawObject(0,0);
--	self:DrawObject(1,1);
--	self:EnablePhysics(0);	
--	self:RemoveDecals();
	
	self:SetTimer(30);
	
end

function BreakableObject:OnDamage(hit)
	if (self.dead == 1) then return end;
	
	if (self.Properties.Physics.bRigidBody) then
		self:AwakePhysics(1);
		if( hit.ipart and hit.ipart>=0 ) then
			self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
		end
	end
		
	--System.Log("\001 active OnDamage "..self.curr_damage);
	if( hit )then
		self.curr_damage=self.curr_damage-hit.damage;
	end	
	if(self.curr_damage<=0)then
		self:GoDead(hit.dir);
		self.NoDecals = 1;
		hit.target_material = nil;
	end
end

BreakableObject.Active={
	OnBeginState=function(self)
		--System.Log("enter alive");
	end,
	

	OnContact = function(self,collider)

		
--System.Log("\001ON contact ");				
		-- it it's vehicle
		if( collider.driverT and self.bBreakByCar==1 ) then
			self:GoDead();
		end
	end,
	OnDamage = BreakableObject.OnDamage,
	
	------------------------------------------------------------------------------------------------------
	-- Use Basic Entity function for this.
	------------------------------------------------------------------------------------------------------
	OnCollide = BasicEntity.OnCollide,
}

BreakableObject.Dead={
	OnBeginState=function(self)
		--System.Log("enter dead");
--		self:SetTimer(1);
		self.dead = 1;
		
		if(self.isLoading == nil) then
			self:Event_IsDead();
			self:BreakEntity(self.deathVector, self.Properties.fBreakImpuls,self.Properties.Parts.bRigidBody,self.Properties.Parts.LifeTime,self.Properties.Parts.Density );
		end	
		
		self:DrawObject(0,0);
		self:DrawObject(1,1);
		self:EnablePhysics(0);	
	end,

	OnTimer=function(self)
		self:RemoveDecals();
	end,
}


function BreakableObject:OnSave(props)
	--WriteToStream(stm,self.Properties);
	--stm:WriteInt(self.dead);
	props.dead = self.dead
end


function BreakableObject:OnLoad(props)
	self.dead = props.dead;
	
	if(self.dead == 0) then
		self:GoAlive();
	else
		self.isLoading = 1;
		self:GoDead();
	end
end

BreakableObject.FlowEvents =
{
	Inputs =
	{
		Die = { BreakableObject.Event_Die, "bool" },
		IsDead = { BreakableObject.Event_IsDead, "bool" },
	},
	Outputs =
	{
		Die = "bool",
		IsDead = "bool",
	},
}

MakeUsable(BreakableObject);
MakePickable(BreakableObject);
MakeTargetableByAI(BreakableObject);
MakeKillable(BreakableObject);
