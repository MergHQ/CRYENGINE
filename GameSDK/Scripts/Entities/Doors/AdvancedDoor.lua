--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2006.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Improved door class for more interaction
--
--------------------------------------------------------------------------
--  History:
--  - 7/2006     : Created by Sascha Gundlach
--
--------------------------------------------------------------------------

AdvancedDoor =
{
	Client = {},
	Server = {},
	PropertiesInstance =
	{
		bLocked = 0,
	},
	Properties=
	{
		soclasses_SmartObjectClass = "Door",
		fileModel = "Objects/architecture/buildings/fishing_houses/fishing_door_c.cgf",
		ModelSubObject = "",
		fileModelDestroyed = "",
		DestroyedSubObject = "",
		fHealth = 500,
		Mass = 50,
		bUsePortal = 1,
		Limits =
		{
			fUseDistance = 2,
			OpeningRange = 90,
			MaxForce = 0,
			MaxBend = 0,
			Damping = 0,
			fSpeed = 1,
			fAutoCloseTime = 0,
			bOpenFromFront = 1,
			bOpenFromBack = 1,
			InitialAngle = 0,
			bIsBreachable = 1,
		},
		Breakage =
		{
			fLifeTime = 10,
			fExplodeImpulse = 0,
			bSurfaceEffects = 1,
		},
		Destruction =
		{
			bExplode = 1,
			ParticleEffect = "breakable_objects.tree_break.small",
			EffectScale = 1,
			Radius = 1,
			Pressure = 12,
			Damage = 0,
			Decal = "",
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
		Sound =
		{
			soundOpenSound   = "Sounds/physics:objects/doors:metal_wire_door_open",
			soundCloseSound  = "Sounds/physics:objects/doors:metal_wire_door_close",
			soundLockedSound = "Sounds/physics:objects/doors:metal_wire_door_locked",
		}
	},
	LastHit =
	{
		impulse = {x=0,y=0,z=0},
		pos     = {x=0,y=0,z=0},
	},

	Editor=
	{
		Icon = "door.bmp",
		ShowBounds = 1,
	},
	States = {"Opened","Closed","Destroyed"},
	advancedDoor = true,
}

function AdvancedDoor:OnReset()
	AI.SetSmartObjectState( self.id, "Closed" );

	local props=self.Properties;
	if(not EmptyString(props.fileModel))then
		self:LoadSubObject(0,props.fileModel,props.ModelSubObject);
	end;

	if(not EmptyString(props.fileModelDestroyed))then
		self:LoadSubObject(1, props.fileModelDestroyed,props.DestroyedSubObject);
	elseif(not EmptyString(props.DestroyedSubObject))then
		self:LoadSubObject(1,props.fileModel,props.DestroyedSubObject);
	end;


	--Limit range to reasonable values
	self.openingrange=props.Limits.OpeningRange;
	self.openingrange=clamp(props.Limits.OpeningRange,-175,175);

	self:GetAngles(self.rad);
	self:GetAngles(self.initialrot);
	self:GetPos(self.initialpos);
	self.angle.z=(self.rad.z*g_Rad2Deg);
	self.defaultangle=self.angle.z;
	self:UpdateImpulsePos();
	self:SetCurrentSlot(0);
	self:PhysicalizeThis(0,0);
	self:EnablePhysics(true);
	self:AwakePhysics(0);
	self:Activate(0);
	self.bUpdate=0;
	self.lasttime=0;
	self.bOpenNow = 0;
	self.bBreachNow=0;
	self.lockbroken=0;
	self.openpercentage=0;
	self.health=props.fHealth;
	self.locked=self.PropertiesInstance.bLocked;

	if(self.Properties.bUsePortal==0)then
		System.ActivatePortal(self:GetWorldPos(),1,self.id);
	end;
	if(self.locked==1)then
		AI.ModifySmartObjectStates( self.id, "Locked" );
	end;
	self:CheckInitalAngle();

end;

function AdvancedDoor:CheckInitalAngle()
	local angle=self.Properties.Limits.InitialAngle;
	if(angle~=0)then
		if((angle<0 and self.openingrange<0) or (angle>0 and self.openingrange>0))then
			self:PhysicalizeThis(0,1);
			self:EnablePhysics(true);
			self:AwakePhysics(1);
			self:Activate(1);
			self:Apply();

			local tmp={x=0,y=0,z=0}
			CopyVector(tmp,self.initialrot);
			tmp.z=tmp.z+(angle*g_Deg2Rad);
			self:SetAngles(tmp);
			self:UpdateImpulsePos();
			if(self.Properties.bUsePortal==1)then
				System.ActivatePortal(self:GetWorldPos(),0,self.id);
			end;
			self:GotoState("Opened");
		end;
	else
		self:GotoState("Closed");
	end;
end;

function AdvancedDoor:GetOpenPercentage()
	return self.openpercentage;
end;

-- Update position of impulse application
function AdvancedDoor:UpdateImpulsePos()
	self.impulsepos=self:GetWorldPos();
	local dirX=self:GetDirectionVector(0);
	local dirY=self:GetDirectionVector(1);
	local dirZ=self:GetDirectionVector(2);
	local offset={x=0,y=0,z=0};
	local bbmin,bbmax = self:GetLocalBBox();
	if(bbmax.x>0.1)then
		self.leftsided=0;
		offset.x=bbmax.x;
	else
		self.leftsided=1;
		offset.x=bbmin.x;
	end;
	offset.y=bbmax.y/2;
	offset.z=bbmax.z/2;
	self.impulsepos.x = self.impulsepos.x + dirX.x * offset.x + dirY.x * offset.y + dirZ.x * offset.z;
	self.impulsepos.y = self.impulsepos.y + dirX.y * offset.x + dirY.y * offset.y + dirZ.y * offset.z;
	self.impulsepos.z = self.impulsepos.z + dirX.z * offset.x + dirY.z * offset.y + dirZ.z * offset.z;
end;


--Set up constraint parameters
function AdvancedDoor:Apply()
	local ConstraintParams = {pivot={},frame0={},frame1={}};
	CopyVector(ConstraintParams.pivot,self.initialpos);
	CopyVector(ConstraintParams.frame1,self.initialrot);
	CopyVector(ConstraintParams.frame0, ConstraintParams.frame1);
	ConstraintParams.frame0_inner={x=0,y=90,z=0};
	ConstraintParams.frame1_inner={x=0,y=90,z=0};

	if(self.openingrange>0)then
		ConstraintParams.xmin = -self.openingrange;
		ConstraintParams.xmax = 0;
	else
		ConstraintParams.xmin = 0;
		ConstraintParams.xmax = -self.openingrange;
	end;
	ConstraintParams.yzmin = 0;
	ConstraintParams.yzmax = 0;
	ConstraintParams.ignore_buddy = 1;
	ConstraintParams.damping = self.Properties.Limits.Damping;
	ConstraintParams.sensor_size = self.Properties.radius;
	ConstraintParams.max_pull_force = self.Properties.Limits.MaxForce;
	ConstraintParams.max_bend_torque = self.Properties.Limits.MaxBend;
	idConstr = self:SetPhysicParams(PHYSICPARAM_CONSTRAINT, ConstraintParams);
end

function AdvancedDoor:OnUpdate()
	if(self.bUpdate==1)then
		if(self.bWasReset==1)then
			self:PhysicalizeThis(0,1);
			self:EnablePhysics(true);
			self:AwakePhysics(1);
			self:Activate(1);
			self:Apply();
			self.bWasReset=0;
		elseif(self.bOpenNow==1)then
			self:OpenDoor(self.user,self.idx);
			self.bOpenNow=0;
		end;

		self:GetAngles(self.rad);
		self.angle.z=(self.rad.z*g_Rad2Deg);

		if(self.angle.z==self.lastangle)then
			if(self.lasttime==0)then
				self.lasttime=System.GetCurrTime();
			end;
			if((System.GetCurrTime()-self.lasttime)>5.0)then
				self.lasttime=0;
				self.bUpdate=0;
			end;
		end;
		self.lastangle=self.angle.z;

		self.openpercentage=math.abs(self.angle.z-self.defaultangle)/((math.abs(self.openingrange))/100);

		--Set door states
		--if(math.abs(self.angle.z-self.defaultangle)<0.5 and self:GetState()~="Closed" and self.lockbroken==0)then
		if(math.abs(self.angle.z-self.defaultangle)<0.5 and self:GetState()~="Closed")then
			self:Play(0);
			self:GotoState("Closed");
		elseif(math.abs(self.angle.z-self.defaultangle)>0.5 and self:GetState()~="Opened")then
			self:GotoState("Opened");
		elseif(360-math.abs(self.angle.z-self.defaultangle)<0.5 and self:GetState()~="Closed")then
			self:SetAngles(self.initialrot);
			self:Play(0);
			self:GotoState("Closed");
		end;
		if(self.Properties.Limits.fAutoCloseTime~=0 and self:GetState()=="Opened")then
			if(self.iAutoCloseTimer==nil)then
				self.iAutoCloseTimer=Script.SetTimerForFunction(self.Properties.Limits.fAutoCloseTime*1000,"AdvancedDoor.AutoClose",self);
			end;
		end;
	else
		self:GetAngles(self.rad);
		self.angle.z=(self.rad.z*g_Rad2Deg);
		if(self.angle.z~=self.lastangle)then
			self.bUpdate=1;
		end;
		self.lastangle=self.angle.z;
	end;
end

AdvancedDoor.AutoClose=function(self)
	if(self:GetState()=="Opened")then
		self:Close();
	end;
end;

function AdvancedDoor:PhysicalizeThis(slot,bRigidBody)
	if(bRigidBody~=1)then
		self.physics.Mass= -1;
		self.physics.Density= -1;
	else
		self.physics.Mass=self.Properties.Mass;
	end;
	self.physics.bCanBreakOthers = 1;

	EntityCommon.PhysicalizeRigid(self,slot,self.physics,bRigidBody);
	self:SetPhysicParams(PHYSICPARAM_VELOCITY,{v={x=0,y=0,z=0}});
end;

function AdvancedDoor:SetCurrentSlot(slot)
	if (slot == 0) then
		self:DrawSlot(0, 1);
		self:DrawSlot(1, 0);
	else
		self:DrawSlot(0, 0);
		self:DrawSlot(1, 1);
	end
	self.currentSlot = slot;
end

function AdvancedDoor:OnPropertyChange()
	self:OnReset();
end;

function AdvancedDoor.Server:OnInit()
	self.bUpdate = 1;
	self.bWasReset = 0;
	self.bWasDetached = 0;
	self.idConstr = -1;
	self.health = 0;
	self.rad = {};
	self.angle ={};
	self.lasttime =0;
	self.lastangle = 0;
	self.locked = 0;
	self.span = 0;
	self.openingrange = 0;
	self.defaultangle = 0;
	self.bOpenNow=0;
	self.bBreachNow=0;
	self.user = 0;
	self.idx =0;
	self.sndid = nil;
	self.lockbroken=0;
	self.leftsided=0;
	self.iAutoCloseTimer = nil;
	self.initialrot = {x=0,y=0,z=0};
	self.initialpos = {x=0,y=0,z=0};
	self.impulsedir = {x=0,y=0,z=0};
	self.impulsepos = {x=0,y=0,z=0};
	self.tmp = {x=0,y=0,z=0};
	self.tmp_2 = {x=0,y=0,z=0};
	self.tmp_3 = {x=0,y=0,z=0};
	self.physics = 
	{
		bRigidBody=1,
		bRigidBodyActive = 1,
		bResting = 1,
		Density = -1,
		Mass = 100,
		Buoyancy =
		{
			water_density = 1000,
			water_damping = 0,
			water_resistance = 1000,
		},
	};
	self.initialdir=self:GetDirectionVector(1);
	self:OnReset();
end;

----------------------------------------------------------------------------------------------------
function AdvancedDoor:IsDead()
	if(self.health<=0)then
		return true;
	else
		return false;
	end;
end;

function AdvancedDoor.Server:OnHit(hit)
	if(self:GetState()=="Destroyed")then return;end;
	self.bUpdate=1;
	local damage=hit.damage;

	local vul=self.Properties.Vulnerability;
	local pass=hit.damage >= self.Properties.Vulnerability.fDamageTreshold;
	if (pass and hit.explosion) then pass = NumberToBool(vul.bExplosion);
	elseif (pass and hit.type=="collision") then pass = NumberToBool(vul.bCollision);
	elseif (pass and hit.type=="bullet") then pass = NumberToBool(vul.bBullet);
	elseif (pass and hit.type=="melee") then pass = NumberToBool(vul.bMelee);
	elseif (pass) then pass = NumberToBool(vul.bOther); end

	if(pass)then
		self.shooterId=hit.shooterId;
		BroadcastEvent( self,"Hit" );
		if(hit.type=="collision")then
			damage=self:CheckCollision(hit);
		end;
		self.health=self.health-damage;
		if(self:IsDead())then
			self:GotoState("Destroyed");
		end;
	end;

	if(hit.type=="melee" and NumberToBool(vul.bMelee))then
		self.shooterId=hit.shooterId;
		local ent=System.GetEntity(self.shooterId);

		damage=damage/4;
		if(damage>80)then
			if(self:GetState()=="Closed")then
				self:BreachDoor();
			end;
		end;
	end;

end;

function AdvancedDoor:CheckCollision(hit)
	CopyVector(self.LastHit.pos, hit.pos);
	CopyVector(self.LastHit.impulse, hit.dir);
	self.LastHit.impulse.x = self.LastHit.impulse.x * hit.damage;
	self.LastHit.impulse.y = self.LastHit.impulse.y * hit.damage;
	self.LastHit.impulse.z = self.LastHit.impulse.z * hit.damage;

	local tmp=hit.weapon;
	local force=(LengthVector(tmp:GetVelocity())*tmp:GetMass())/1.5;
	return force;

end;

function AdvancedDoor:BreachDoor(user)
		if((self.Properties.bIsBreachable==0) or not self:CheckBreachDirection(user))then return;end;
		self:Activate(1);
		self.bUpdate=1;
		self.bWasReset=1;
		self.locked=0;
		self.bBreachNow=1;
		self.lockbroken=1;
		self:Open(nil,0);
		self:ActivateOutput("Breached",1);
end;

function AdvancedDoor.Client:OnHit(hit, remote)
	CopyVector(self.LastHit.pos, hit.pos);
	CopyVector(self.LastHit.impulse, hit.dir);
	self.LastHit.impulse.x = self.LastHit.impulse.x * hit.damage;
	self.LastHit.impulse.y = self.LastHit.impulse.y * hit.damage;
	self.LastHit.impulse.z = self.LastHit.impulse.z * hit.damage;

	--Change
	self:AddImpulse(-1, hit.pos, hit.dir, 20, 1);

end

function AdvancedDoor:Event_ConstraintBroken(sender)
	BroadcastEvent( self,"ConstraintBroken");
end

function AdvancedDoor:OnSave(tbl)
	tbl.idConstr=self.idConstr;
	tbl.bWasDetached=self.bWasDetached;
	tbl.bWasReset=self.bWasReset;
	tbl.health=self.health;
	tbl.rad=self.rad;
	tbl.angle=self.angle;
	tbl.span=self.span;
	tbl.opeingrange=self.openingrange;
	tbl.defaultangle=self.defaultangle;
	tbl.locked=self.locked;
	tbl.bOpenNow=self.bOpenNow;
	tbl.bBreachNow=self.bBreachNow;
	tbl.user=self.user;
	tbl.idx=self.idx;
	tbl.lockbroken=self.lockbroken;
	tbl.iAutoCloseTimer=self.iAutoCloseTimer;
	tbl.initialpos=self.initialpos;
	tbl.initialrot=self.initialrot;
	tbl.impulsedir=self.impulsedir;
	tbl.leftsided=self.leftsided;
	tbl.impulsepos=self.impulsepos;
end;

function AdvancedDoor:OnLoad(tbl)
	self.idConstr=tbl.idConstr;
	self.bWasDetached=tbl.bWasDetached;
	self.bWasReset=tbl.bWasReset;
	self.health=tbl.health;
	self.rad=tbl.rad;
	self.angle=tbl.angle;
	self.span=tbl.span;
	self.openingrange=tbl.opeingrange;
	self.defaultangle=tbl.defaultangle;
	self.locked=tbl.locked;
	self.bOpenNow=tbl.bOpenNow;
	self.bBreachNow=tbl.bBreachNow;
	self.user=tbl.user;
	self.idx=tbl.idx;
	self.lockbroken=tbl.lockbroken;
	self.iAutoCloseTimer=tbl.iAutoCloseTimer;
	self.initialpos=tbl.initialpos;
	self.initialrot=tbl.initialrot;
	self.impulsedir=tbl.impulsedir;
	self.leftsided=tbl.leftsided;
	self.impulsepos=tbl.impulsepos;

end;

function AdvancedDoor:Play(open)
	--REINST
	-- if(Sound.IsPlaying(self.sndid))then
	-- else
		-- self.sndid=nil;
		-- local snd;
		-- local sndFlags=bor(SOUND_DEFAULT_3D, 0);
		-- if(open==1)then
			-- if(self.locked==0)then
				-- snd=self.Properties.Sound.soundOpenSound;
			-- else
				-- snd=self.Properties.Sound.soundLockedSound;
			-- end;
		-- else
			-- snd=self.Properties.Sound.soundCloseSound;
		-- end;
		-- self.sndid=self:PlaySoundEvent(snd,g_Vectors.v000,g_Vectors.v010,sndFlags,0,SOUND_SEMANTIC_MECHANIC_ENTITY);
	-- end;
end;

function AdvancedDoor:CheckBreachDirection(user)
	if(user~=nil)then
		local limits=self.Properties.Limits;
		CopyVector(self.tmp,self.initialdir);
		self:GetWorldPos(self.tmp_2);
		user:GetWorldPos(self.tmp_3)
		SubVectors(self.tmp_2,self.tmp_2,self.tmp_3);
		NormalizeVector(self.tmp_2);
		local dot=dotproduct3d(self.tmp,self.tmp_2);

		if(self.openingrange>0)then
			if(dot<0)then
				if(self.leftsided==0)then
					return false;
				else
					return true;
				end;
			else
				if(self.leftsided==0)then
					return true;
				else
					return false;
				end;
			end;
		else
			if(dot>0)then
				if(self.leftsided==0)then
					return false;
				else
					return true;
				end;
			else
				if(self.leftsided==0)then
					return true;
				else
					return false;
				end;
			end;
		end;

	end;
end;

function AdvancedDoor:CheckUseDirection(user)
	--if(self.lockbroken==1)then
		--return true;
	--end;
	local limits=self.Properties.Limits;
	CopyVector(self.tmp,self.initialdir);
	self:GetWorldPos(self.tmp_2);
	user:GetWorldPos(self.tmp_3)
	SubVectors(self.tmp_2,self.tmp_2,self.tmp_3);
	NormalizeVector(self.tmp_2);
	local dot=dotproduct3d(self.tmp,self.tmp_2);
	if(dot>0)then
		if(limits.bOpenFromFront==0)then
			return false;
		end;
	else
		if(limits.bOpenFromBack==0)then
			return false;
		end;
	end;
	return true;
end;

function AdvancedDoor:OnUsed(user, idx)
	if(self:GetState()=="Closed")then
		if(self:CheckUseDirection(user))then
			if(self.locked==1)then
				self:Play(1);
				return;
			else
				self:Use(user, idx);
			end;
		end;
	else
		self:Use(user, idx);
	end;
end;

function AdvancedDoor:Use(user, idx)
	self:Activate(1);
	self.bUpdate=1;
	self.lasttime=0;
	Log("self:GetState(): "..self:GetState());
	if(self:GetState()=="Closed")then
		self:Open(user,idx);
	else
		self:Close(user,idx);
	end;
end;

function AdvancedDoor:Open(user,idx)
	if(self:GetState()=="Closed")then
		self:SetPos(self.initialpos);
		self:SetAngles(self.initialrot);
		self.bWasReset=1;
		self.bOpenNow=1;
		self.user=user;
		self.idx=idx;
	end;
end;

function AdvancedDoor:OpenDoor(user,idx)
	self.impulsedir=self:GetDirectionVector(1);
	if(self.openingrange>0)then
		if(self.leftsided==1)then
			ScaleVectorInPlace(self.impulsedir,-1);
		end;
	else
		if(self.leftsided==0)then
			ScaleVectorInPlace(self.impulsedir,-1);
		end;
	end;

	local strength;
	local force;
	--Change
	if(user and user==g_localActor)then
		force=self.Properties.Mass;
	else
		if(self.bBreachNow==1)then
			force=self.Properties.Mass*4;
			bBreachNow=0;
		else
			force=self.Properties.Mass;
		end;
	end;
	force=force*self.Properties.Limits.fSpeed;

	self:UpdateImpulsePos();
	self:AddImpulse(-1,self.impulsepos,self.impulsedir,force,1);
end;

function AdvancedDoor:Close(user,idx)
	self.bUpdate=1;
	self.lasttime=0;
	self.impulsedir=self:GetDirectionVector(1);
	ScaleVectorInPlace(self.impulsedir,-1);

	if(self.openingrange>0)then
		if(self.leftsided==1)then
			ScaleVectorInPlace(self.impulsedir,-1);
		end;
	else
		if(self.leftsided==0)then
			ScaleVectorInPlace(self.impulsedir,-1);
		end;
	end;

	local strength;
	local force;
	--Change
	if(user and user==g_localActor)then
		force=self.Properties.Mass;
	else
		if(self.bBreachNow==1)then
			force=self.Properties.Mass*4;
			bBreachNow=0;
		else
			force=self.Properties.Mass;
		end;
	end;
	force=force*self.Properties.Limits.fSpeed;
	self:UpdateImpulsePos();
	self:AddImpulse(-1,self.impulsepos,self.impulsedir,force,1);
end;


function AdvancedDoor:IsUsable(user)
	if(self:GetState()~="Destroyed")then
		--SubVectors(self.tmp,self:GetPos(),user:GetPos());

		self:GetPos(self.tmp_2)
		user:GetPos(self.tmp_3)
		SubVectors(self.tmp,self.tmp_2,self.tmp_3);

		local dist=LengthVector(self.tmp);
		if(dist<self.Properties.Limits.fUseDistance)then
			return 2;
		else
			return 0;
		end;
	else
		return 0;
	end;

end;

function AdvancedDoor:GetUsableMessage(idx)
	if(self.Properties.bUsable==1)then
		return self.Properties.UseMessage;
	else
		return "@use_door";
	end;
end;

function AdvancedDoor:OnShutDown()
end

function AdvancedDoor:Explode()
	local props=self.Properties;
	local hitPos = self.LastHit.pos;
	local hitImp = self.LastHit.impulse;

	self:BreakToPieces(
		0, 0,
		props.Breakage.fExplodeImpulse,
		hitPos,
		hitImp,
		props.Breakage.fLifeTime,
		props.Breakage.bSurfaceEffects
	);
	if(NumberToBool(self.Properties.Destruction.bExplode))then
		local explosion=self.Properties.Destruction;
		g_gameRules:CreateExplosion(self.shooterId, self.id, explosion.Damage, self:GetWorldPos(), explosion.Direction, explosion.Radius, nil, explosion.Pressure, explosion.HoleSize, explosion.ParticleEffect, explosion.EffectScale);
	end;
	self:RemoveDecals();
	self:SetCurrentSlot(1);
	self:PhysicalizeThis(1,1);
	self:AwakePhysics(1);
end;

function AdvancedDoor:Lock()
	AI.ModifySmartObjectStates( self.id, "Locked" );
	self.locked=1;
end;

function AdvancedDoor:Unlock()
	AI.ModifySmartObjectStates( self.id, "-Locked" );
	self.locked=0;
end;

----------------------------------------------------------------------------------
------------------------------------States----------------------------------------
----------------------------------------------------------------------------------
AdvancedDoor.Server.Destroyed=
{
	OnBeginState = function( self )
		AI.ModifySmartObjectStates( self.id, "Destroyed-Closed,Open,Locked" );
		self:Explode();
		BroadcastEvent(self, "Destroyed")
	end,
	OnUpdate = function(self, dt)
	end,
	OnEndState = function( self )

	end,
}
AdvancedDoor.Server.Closed=
{
	OnBeginState = function( self )
		if(self.iAutoCloseTimer~=nil)then
			Script.KillTimer(self.iAutoCloseTimer);
			self.iAutoCloseTimer=nil;
		end;
		AI.ModifySmartObjectStates( self.id, "Closed-Open" );
		self:PhysicalizeThis(0,0);
		self:EnablePhysics(true);
		self:AwakePhysics(0);
		self:Activate(0);
		self.bUpdate=0;
		self.lasttime=0;
		if(self.Properties.bUsePortal==1)then
			System.ActivatePortal(self:GetWorldPos(),0,self.id);
		end;
	end,
	OnUpdate = function(self, dt)
		self:OnUpdate();
	end,
	OnEndState = function( self )
	end,
}
AdvancedDoor.Server.Opened=
{
	OnBeginState = function( self )
		if(self.Properties.bUsePortal==1)then
			System.ActivatePortal(self:GetWorldPos(),1,self.id);
		end;
		self.bUpdate=1;
		self.lasttime=0;
		AI.ModifySmartObjectStates( self.id, "Open-Closed" );
		self:Play(1);
	end,

	OnUpdate = function(self, dt)
		self:OnUpdate();
	end,
	OnEndState = function( self )

	end,
}
----------------------------------------------------------------------------------
------------------------------------Events----------------------------------------
----------------------------------------------------------------------------------
function AdvancedDoor:Event_Unlock()
	self:Unlock();
	BroadcastEvent(self, "Unlock");
end;

function AdvancedDoor:Event_Lock()
	self:Lock();
	BroadcastEvent(self, "Lock");
end;

function AdvancedDoor:Event_Open()
	self.bUpdate=1;
	self.bWasReset=1;
	self:Open();
	BroadcastEvent(self, "Open");
end;

function AdvancedDoor:Event_Close()
	self:Close();
	BroadcastEvent(self, "Close");
end;

function AdvancedDoor:Event_Destroyed()
	self.locked=0;
	self:GotoState("Destroyed");
end;

-----------------------------------------------------------------------------------

AdvancedDoor.FlowEvents =
{
	Inputs =
	{
		Unlock = { AdvancedDoor.Event_Unlock, "bool" },
		Lock = { AdvancedDoor.Event_Lock, "bool" },
		Open = { AdvancedDoor.Event_Open, "bool" },
		Close = { AdvancedDoor.Event_Close, "bool" },
		Destroyed = { AdvancedDoor.Event_Destroyed, "bool" },
	},
	Outputs =
	{
		Unlock = "bool",
		Lock = "bool",
		Open = "bool",
		Close = "bool",
		Destroyed = "bool",
		Breached = "bool",
	},
}