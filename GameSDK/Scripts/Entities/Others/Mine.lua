--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2006.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Mine entity
--
--------------------------------------------------------------------------
--  History:
--  - 7/2006     : Created by Sascha Gundlach
--
--------------------------------------------------------------------------

Mine =
{
	Client = {},
	Server = {},
	Properties =
	{
		fileModel = "objects/weapons/ammo/avmine/avmine.cgf",
		Radius = 2,
		bAdjustToTerrain = 1,
		Options =
		{
			fDisarmDistance = 0.5,
			fMinTriggerWeight = 50,
			bNoVehicles = 0,
			bWaterMine = 0,
		},
		FrogMine =
		{
			bIsFrogMine = 0,
			fJumpHeight = 2.5,
			bJumpWhenShot = 1,
			fDetonationDelay = 0.75,
		},
		Claymore =
		{
			bIsClaymore = 0,
			fCone = 45,
			bCenterExplosion = 1,
			fDamage = 100,
		},
		Destruction =
		{
			ParticleEffect = "explosions.rocket.soil",
			EffectScale = 0.2,
			Radius = 5,
			Pressure = 2500,
			Damage = 500,
			Direction = {x=0, y=0, z=-1},
		},
		Vulnerability =
		{
			fDamageTreshold = 0,
			bExplosion = 1,
			bCollision = 1,
			bMelee = 1,
			bBullet = 1,
			bOther = 1,
		},
	},
	LastHit =
	{
		impulse = {x=0,y=0,z=0},
		pos = {x=0,y=0,z=0},
	},

	Editor =
	{
		Icon = "mine.bmp",
		IconOnTop = 1,
		ShowBounds = 1,
	},

	States = {"Deactivated","Armed","Disarmed","Destroyed"},
}
-----------------------------------------------------------------------------------------
MINE_CHECK = 1;
MINE_JUMP  = 2;
-----------------------------------------------------------------------------------------
function Mine:OnReset()
	local props=self.Properties;
	if(not EmptyString(props.fileModel))then
		self:LoadObject(0,props.fileModel);
	end;
	local Min={x=-props.Radius/2,y=-props.Radius/2,z=-2.5/2};
	local Max={x=props.Radius/2,y=props.Radius/2,z=2.5/2};
	self:SetTriggerBBox(Min,Max);
	EntityCommon.PhysicalizeRigid(self,0,self.physics,0);
	--change
	self:SetCurrentSlot(0);
	if(self.Properties.Claymore.bIsClaymore==0 and self.Properties.Options.bWaterMine==0)then
		--Disable for now
		--self:SetViewDistRatio(12);--12
	end;
	self.health=1;
	self.ents={};

	if(self.Properties.bAdjustToTerrain==1)then
		if(self.Properties.Claymore.bIsClaymore==0)then
	 		local pos={x=0,y=0,z=0};
	 		CopyVector(pos,self:GetPos());
	 		pos.z=System.GetTerrainElevation(self:GetPos())-0.01;--better put offset into properties
	 		self:SetPos(pos);
	 	end;
	end;
	self:GotoState("Armed");
end;

function Mine:OnSave(tbl)
	tbl.health=self.health;
	tbl.ents=self.ents;
	tbl.disarmed=self.disarmed;
end;

function Mine:OnLoad(tbl)
	self.health=tbl.health;
	self.ents=tbl.ents;
	self.disarmed=tbl.disarmed;
	EntityCommon.PhysicalizeRigid(self,0,self.physics,0);
	self:SetCurrentSlot(0);
end;


function Mine:OnPropertyChange()
	self:OnReset();
end;

function Mine.Server:OnInit()
	local props=self.Properties.FrogMine;
	if(props.bIsFrogMine==1)then
		self.physics = 
		{
			bRigidBody = 1,
			bRigidBodyActive = 1,
			bResting = 1,
			Density= -1,
			Mass = 20,
		};
	else
		self.physics = 
		{
			bRigidBody = 0,
			bRigidBodyActive = 0,
			bResting = 1,
			Density= -1,
			Mass= 20 ,
		};
	end;

	self.health=1;
	self.ents={};
	self.disarmed=0;
	self.tmp={x=0,y=0,z=0};
	self.tmp_2={x=0,y=0,z=0};
	self.tmp_3={x=0,y=0,z=0};
	self:OnReset();
end;

function Mine:IsUsable(user)
	if((self:GetState()~="Destroyed") and (self:GetState()~="Disarmed"))then
		self:GetPos(self.tmp_2);
		user:GetPos(self.tmp_3);
		SubVectors(self.tmp,self.tmp_2,self.tmp_3);
		local dist=LengthVector(self.tmp);
		if(dist<self.Properties.Options.fDisarmDistance)then
			return 2;
		else
			return 0;
		end;
	else
		return 0;
	end;
end;

function Mine:GetUsableMessage(idx)
	return "Press USE to disarm!";
end;

function Mine:OnUsed()
	self:GotoState("Disarmed");
end;

function Mine.Server:OnHit(hit)
	if((self:GetState()=="Disarmed") or (self:GetState()=="Destroyed") or (self:GetState()=="Deactivated"))then
		return;
	end;
	local vul=self.Properties.Vulnerability;
	local pass=hit.damage >= self.Properties.Vulnerability.fDamageTreshold;
	if (pass and hit.explosion) then
		pass = NumberToBool(vul.bExplosion);
		--Ignore damage from claymores to other mines
		if(hit.shooterId and System.GetEntityClass(hit.shooterId)=="Mine")then
			pass=false;
		end;
	elseif (pass and hit.type=="collision") then pass = NumberToBool(vul.bCollision);
	elseif (pass and hit.type=="bullet") then pass = NumberToBool(vul.bBullet);
	elseif (pass and hit.type=="melee") then pass = NumberToBool(vul.bMelee);
	elseif (pass) then pass = NumberToBool(vul.bOther); end
	if(pass)then
		local damage= hit.damage;
		self.shooterId=hit.shooterId;
		self.health=self.health-damage;
		if(self.health<=0)then
			self:GotoState("Destroyed");
		end;
	end;
end;


function Mine:IsDead()
	if(self.health<=0)then
		return true;
	else
		return false;
	end;
end;

function Mine:SetCurrentSlot(slot)
	if(slot==0)then
		self:DrawSlot(0, 1);
		self:DrawSlot(1, 0);
	else
		self:DrawSlot(0, 0);
		self:DrawSlot(1, 1);
	end
	self.currentSlot = slot;
end

function Mine:Explode()
	self:Hide(1);
	local props=self.Properties;
	local explosion=props.Destruction;
	local radius=explosion.Radius;
	--Raise the radius a bit if it is a frog mine
	if(props.FrogMine.bIsFrogMine==1)then
		radius=radius+1.5;
	end;
	if(props.Claymore.bIsClaymore==0)then
		g_gameRules:CreateExplosion(self.id, self.id, explosion.Damage, self:GetWorldPos(), explosion.Direction, radius, nil, explosion.Pressure, explosion.HoleSize, explosion.ParticleEffect, explosion.EffectScale);
	else
		--Claymore
		local angle=props.Claymore.fCone*g_Deg2Rad;
		if(props.Claymore.bCenterExplosion==1)then
			g_gameRules:CreateExplosion(self.id, self.id, explosion.Damage, self:GetWorldPos(), explosion.Direction, 1, nil, 0, 0, "", explosion.EffectScale);
		end;
		g_gameRules:CreateExplosion(self.id, self.id, explosion.Damage, self:GetWorldPos(), self:GetDirectionVector(1), radius, angle, explosion.Pressure, explosion.HoleSize, explosion.ParticleEffect, explosion.EffectScale);
	end;

----------------------------------------------------
-- DECAL SUPPORT DEPRECATED - Use Material Effects
----------------------------------------------------
--	if (explosion.Decal ~= "") then
--		local decalDir = explosion.Direction;
--		if (props.Claymore.bIsClaymore~=0) then
--			decalDir = g_Vectors.down;
--		end
--		Particle.CreateDecal(self:GetWorldPos(), decalDir, radius*explosion.DecalScale, 300, explosion.Decal, math.random()*360, decalDir);
--	end
--	self:RemoveDecals();
end;

function Mine:Jump()
	--Disable for now
	--self:SetViewDistRatio(100);
	EntityCommon.PhysicalizeRigid(self,0,self.physics,1);
	local v0=math.sqrt(2*9.81*self.Properties.FrogMine.fJumpHeight);
	local force=20*v0;
	local impulsepos=g_Vectors.v000;
	CopyVector(impulsepos,self:GetPos());
	impulsepos.x=impulsepos.x+0.005;
	self:AddImpulse(-1,impulsepos,{x=0,y=0,z=1},force,1);
	self:PlaySoundEvent("sounds/weapons:mine:mine_jump",g_Vectors.v000,g_Vectors.v010,SOUND_DEFAULT_3D,0,SOUND_SEMANTIC_MECHANIC_ENTITY);
	self:SetTimer(MINE_JUMP,self.Properties.FrogMine.fDetonationDelay*1000);
end;

function Mine:CheckEntities()
	local speed;
	local tmp={x=0,y=0,z=0};
	local dist;
	local options=self.Properties.Options;
	for i,v in pairs(self.ents) do
		if(v~=nil)then
			if(v:GetMass()<options.fMinTriggerWeight)then
				--Log(v:GetName().." too light, don't trigger!");
				break;
			elseif(options.bNoVehicles==1 and v.vehicle~=nil)then
				--Log(v:GetName().." is no vehicle, don't trigger!");
				break;
			end;
			if(self.Properties.Options.bWaterMine==0)then
				local vel={x=0,y=0,z=0};
				v:GetVelocity(vel);
				speed=LengthVector(vel);
				self:GetPos(self.tmp);
				v:GetPos(self.tmp_2);
				SubVectors(tmp,self.tmp_2,self.tmp_2);
				dist=LengthVector(tmp);
				--Log("Dist: "..dist.." Velocity: "..speed.." ("..0.55*(1+(dist/2.5)).." allowed)");
				if(speed>0.55*(1+(dist/2.5)))then
					--Log("Triggered by: "..v:GetName().." with weight: "..v:GetMass());
					self:GotoState("Destroyed");
					break;
				end;
			else
				--Watermines don't care for speed and will always explode
				if(v:IsEntityInside(self.id))then
					self:GotoState("Destroyed");
					break;
				end;
			end;
		end;
	end;
	if(table.getn(self.ents)~=0)then
		self:SetTimer(MINE_CHECK,100);
	end;
end;


-----------------------------------------------------------------------------------
function Mine:Event_Disarmed()
	self:GotoState("Disarmed");
	BroadcastEvent(self, "Disarmed")
end;

function Mine:Event_Detonated()
	self:GotoState("Destroyed");
	BroadcastEvent(self, "Detonated")
end;

function Mine:Event_Activated()
	if(self.disarmed==0)then
		self:GotoState("Armed");
	end;
	BroadcastEvent(self, "Activated")
end;

function Mine:Event_Deactivated()
	self:GotoState("Deactivated");
	BroadcastEvent(self, "Deactivated")
end;


-----------------------------------------------------------------------------------

Mine.Server.Deactivated = {};

Mine.Server.Armed =
{
	OnBeginState = function(self)
		if(self.Properties.Claymore.bIsClaymore==0 and self.Properties.Options.bWaterMine==0)then
			--Disable for now
			--self:SetViewDistRatio(12);
		end;
		BroadcastEvent(self, "Armed")
	end,
	OnEnterArea = function(self,entity,areaId)
		if(entity and entity:GetMass()>=self.Properties.Options.fMinTriggerWeight)then
			if(entity.actor)then
				table.insert(self.ents,entity);
				self:SetTimer(MINE_CHECK,100);
			else
				self:GotoState("Destroyed");
			end;
		end;
	end,
	OnLeaveArea = function(self,entity,areaId)
		for i,v in pairs(self.ents) do
			if(v==entity)then
				table.remove(self.ents,i);
				break;
			end;
		end;
	end,
	OnTimer = function(self,timerId)
		if(timerId==MINE_CHECK)then
			self:CheckEntities();
		end;
	end,
};

Mine.Server.Disarmed =
{
	OnBeginState = function(self)
		self.disarmed=1;
		for i,v in pairs(self.ents) do
			table.remove(self.ents,i);
		end;
		--Disable for now
		--self:SetViewDistRatio(100);
		BroadcastEvent(self, "Disarmed")
	end,
};

Mine.Server.Destroyed =
{
	OnBeginState = function( self )
		local props=self.Properties.FrogMine;
		if(props.bIsFrogMine==1)then
			if(self:IsDead())then
				if(not props.bJumpWhenShot)then
					self:Explode();
				else
					self:Jump();
				end;
			else
				self:Jump();
			end;
		else
			self:Explode();
		end;
		BroadcastEvent(self, "Detonated")
	end,
	OnTimer = function(self,timerId)
		if(timerId==MINE_JUMP)then
			self:Explode();
		end;
	end,
};
-----------------------------------------------------------------------------------

Mine.FlowEvents =
{
	Inputs =
	{
		Detonated = { Mine.Event_Detonated, "bool" },
		Disarmed = { Mine.Event_Disarmed, "bool" },
		Activated = { Mine.Event_Activated, "bool" },
		Deactivated = { Mine.Event_Deactivated, "bool" },
	},
	Outputs =
	{
		Detonated = "bool",
		Disarmed = "bool",
		Activated = "bool",
		Deactivated = "bool",
	},
}
