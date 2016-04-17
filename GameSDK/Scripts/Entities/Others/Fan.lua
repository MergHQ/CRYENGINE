Fan = {
	Client = {},
	Server = {},
	Properties =
	{
		fileModel = "Objects/props/furniture/appliances/ceiling_fan/ceiling_fan.cgf",
		ModelSubObject = "",
		fileModelDestroyed = "",
		DestroyedSubObject = "",
		MaxSpeed = 0.1,
		fHealth = 100,
		bTurnedOn = 1,
		Physics =
		{
			bRigidBody = 0,
			bRigidBodyActive = 0,
			bRigidBodyAfterDeath =1,
			bResting = 1,
			Density = -1,
			Mass = 150,
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
			ParticleEffect = "explosions.barrel.wood_explode",
			EffectScale = 1,
			Radius = 1,
			Pressure = 1,
			Damage = 0,
			Decal = "",
			Direction = {x=0, y=0, z=1},
		},
	},
	Editor =
	{
		Icon = "animobject.bmp",
		IconOnTop = 1,
	},
	States = {"TurnedOn","TurnedOff","Accelerating","Decelerating","Destroyed"},
	fCurrentSpeed = 0,
	fDesiredSpeed = 0,
	LastHit =
	{
		impulse = {x=0,y=0,z=0},
		pos = {x=0,y=0,z=0},
	},
	shooterId = 0,
}


function Fan:OnPropertyChange()
	self:OnReset();
end;

function Fan:OnSave(tbl)
	tbl.fCurrentSpeed=self.fCurrentSpeed;
	tbl.fDesiredSpeed=self.fDesiredSpeed;
end;

function Fan:OnLoad(tbl)
	self.fCurrentSpeed=tbl.fCurrentSpeed;
	self.fDesiredSpeed=tbl.fDesiredSpeed;
end;

function Fan:OnReset()
	local props=self.Properties;
	self.health=props.fHealth;
	if(not EmptyString(props.fileModel))then
		self:LoadSubObject(0,props.fileModel,props.ModelSubObject);
	end;
	if(not EmptyString(props.fileModelDestroyed))then
		self:LoadSubObject(1, props.fileModelDestroyed,props.DestroyedSubObject);
	elseif(not EmptyString(props.DestroyedSubObject))then
		self:LoadSubObject(1,props.fileModel,props.DestroyedSubObject);
	end;
	self:Activate(1);
	self:SetCurrentSlot(0);
	self:PhysicalizeThis(0);
	if(props.bTurnedOn==1)then
		self.fCurrentSpeed=self.Properties.MaxSpeed;
		self:GotoState("TurnedOn");
	else
		self.fCurrentSpeed=0;
		self:GotoState("TurnedOff");
	end;

	self.fDesiredSpeed=self.Properties.MaxSpeed;
end;

function Fan:PhysicalizeThis(slot)
	local physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid( self,slot,physics,1 );
end;

function Fan.Client:OnHit(hit, remote)
	CopyVector(self.LastHit.pos, hit.pos);
	CopyVector(self.LastHit.impulse, hit.dir);
	self.LastHit.impulse.x = self.LastHit.impulse.x * hit.damage;
	self.LastHit.impulse.y = self.LastHit.impulse.y * hit.damage;
	self.LastHit.impulse.z = self.LastHit.impulse.z * hit.damage;
end

function Fan.Server:OnHit(hit)
	self.shooterId=hit.shooterId;
	self.health=self.health-hit.damage;
	BroadcastEvent( self,"Hit" );
	if(self.health<=0)then
		self:GotoState("Destroyed");
	end;
end;

function Fan:Explode()
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
	self:SetCurrentSlot(1);
	if(props.Physics.bRigidBodyAfterDeath==1)then
			local tmp=props.Physics.bRigidBody;
			props.Physics.bRigidBody=1;
			self:PhysicalizeThis(1);
			--props.bPickable=1;
			props.Physics.bRigidBody=tmp;
	else
		self:PhysicalizeThis(1);
	end;
	self:RemoveDecals();
	self:AwakePhysics(1);
end;

function Fan:SetCurrentSlot(slot)
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
function Fan.Server:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized=1;
	end;
end;

----------------------------------------------------------------------------------------------------
function Fan.Client:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized=1;
	end;
end;


----------------------------------------------------------------------------------
------------------------------------Events----------------------------------------
----------------------------------------------------------------------------------
function Fan:Event_Destroyed()
	BroadcastEvent(self, "Destroyed");
end;

function Fan:Event_TurnOn()
	if(self:GetState()~="Destroyed")then
		self:GotoState("Accelerating");
	end;
end;

function Fan:Event_TurnOff()
	if(self:GetState()~="Destroyed")then
		self:GotoState("Decelerating");
	end;
end;

function Fan:Event_Switch()
	if(self:GetState()~="Destroyed")then
		if(self:GetState()=="Accelerating" or self:GetState()=="TurnedOn")then
			self:GotoState("Decelerating");
		elseif(self:GetState()=="Decelerating" or self:GetState()=="TurnedOff")then
			self:GotoState("Accelerating");
		end;
	end;
end;

function Fan:Event_Hit(sender)
	BroadcastEvent( self,"Hit" );
end;


----------------------------------------------------------------------------------
------------------------------------States----------------------------------------
----------------------------------------------------------------------------------
Fan.Server.TurnedOn =
{
	OnBeginState = function( self )
		BroadcastEvent(self, "TurnOn")
	end,
	OnUpdate = function (self, dt)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed*dt/0.025;
		self:SetAngles(g_Vectors.temp_v1);
	end,
	OnEndState = function( self )

	end,
}

Fan.Server.TurnedOff =
{
	OnBeginState = function( self )
		BroadcastEvent(self, "TurnOff")
	end,
	OnEndState = function( self )

	end,
}

Fan.Server.Accelerating =
{
	OnBeginState = function( self )

	end,
	OnUpdate = function(self, dt)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed*dt/0.025;
		self:SetAngles(g_Vectors.temp_v1);

		if(self.fCurrentSpeed<self.fDesiredSpeed)then
			self.fCurrentSpeed=self.fCurrentSpeed+(self.fDesiredSpeed/100)*dt/0.025;
		else
			self.fCurrentSpeed=self.fDesiredSpeed;
			self:GotoState("TurnedOn");
		end;
	end,
	OnEndState = function( self )

	end,
}

Fan.Server.Decelerating =
{
	OnBeginState = function( self )

	end,
	OnUpdate = function(self, dt)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed*dt/0.025;
		self:SetAngles(g_Vectors.temp_v1);
		
		if(self.fCurrentSpeed>0.01)then
			self.fCurrentSpeed=self.fCurrentSpeed-(self.fDesiredSpeed/100)*dt/0.025;
		else
			self:GotoState("TurnedOff");
		end;
	end,
	OnEndState = function( self )

	end,
}

Fan.Server.Destroyed =
{
	OnBeginState = function( self )
		self:Explode();
		BroadcastEvent(self, "Destroyed")
	end,
	OnUpdate = function(self, dt)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed*dt/0.025;
		self:SetAngles(g_Vectors.temp_v1);
		if(self.fCurrentSpeed>0.01)then
			self.fCurrentSpeed=self.fCurrentSpeed-((self.fDesiredSpeed/100)*2)*dt/0.025;
		end;
	end,
	OnEndState = function( self )

	end,
}

----------------------------------------------------------------------------------
-------------------------------Flow-Graph Ports-----------------------------------
----------------------------------------------------------------------------------

Fan.FlowEvents =
{
	Inputs =
	{
		Switch = { Fan.Event_Switch, "bool" },
		TurnOn = { Fan.Event_TurnOn, "bool" },
		TurnOff = { Fan.Event_TurnOff, "bool" },
		Hit = { Fan.Event_hit, "bool" },
		Destroyed = { Fan.Event_Destroyed, "bool" },
	},
	Outputs =
	{
		TurnOn = "bool",
		TurnOff = "bool",
		Hit = "bool",
		Destroyed = "bool",
	},
}

