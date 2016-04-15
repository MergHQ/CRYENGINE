Fan = {
	Client = {},
	Server = {},
	Properties = {
		fileModel 	= "objects/library/props/electronic_devices/fans/ceiling_fan_local.cgf",
		fileDestroyedModel = "objects/library/props/electronic_devices/fans/ceiling_fan_local.cgf",
		MaxSpeed = 0.25,
		fHealth = 25,
		Physics = {
			bRigidBody=1,
			bRigidBodyActive = 1,
			bResting = 1,
			Density = -1,
			Mass = 150,
		},
		Destruction = {
			bExplode	= 1,
			Effect = "explosions.large_Vehicle.a",
			EffectScale = 0.05,
			Radius = 10,
			Pressure	= 12,
			Damage = 25,
			Decal = "",
		},
		Editor={
			Icon = "Item.bmp",
			IconOnTop=1,
		},
		States = {"TurnedOn","TurnedOff","Accelerating","Decelerating","Destroyed"},
		fCurrentSpeed = 0,
		fDesiredSpeed = 0,
	}
}

function Fan:OnPropertyChange()
	self:OnReset();
end;

function Fan:OnSave(tbl)

end;

function Fan:OnLoad(tbl)

end;

function Fan:OnReset()
	self.health=self.Properties.fHealth;
	self:PhysicalizeThis(0);
	self:GotoState("TurnedOff");
	self.fCurrentSpeed=0;
	self.fDesiredSpeed=self.Properties.MaxSpeed;
end;

function Fan:PhysicalizeThis()
	local mdl=self.Properties.fileModel;
	if(mdl~="")then
		self:LoadObject(0,mdl);
	end;
	local physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid( self,0,physics,1 );
end;

function Fan.Server:OnHit(shooterId, weaponId, matName, damage)
	self.health=self.health-damage;
	BroadcastEvent( self,"Hit" );
	if(self.health<=0)then
		self:GotoState("Destroyed");
	end;
	
end;

function Fan:Explode()
	if(NumberToBool(self.Properties.Destruction.bExplode))then
		local explosion=self.Properties.Destruction;
		g_gameRules.server:RequestExplosion(NULL_ENTITY, self.id, self:GetWorldPos(), explosion.Direction,
			explosion.Radius, explosion.Pressure, explosion.Damage, 0, 0, explosion.Decal,
			explosion.Effect, explosion.EffectScale);
	end;
	local mdl=self.Properties.fileDestroyedModel;
	if(mdl~="")then
		self:LoadObject(0,mdl);
	end;
end;

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
		self:SetTimer(0,25);
	end,
	OnTimer = function(self,timerId,msec)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed;
		self:SetAngles(g_Vectors.temp_v1);
		self:SetTimer(0,25);
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
		self:SetTimer(0,25);
	end,
	OnTimer = function(self,timerId,msec)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed;
		self:SetAngles(g_Vectors.temp_v1);
		
		if(self.fCurrentSpeed<=self.fDesiredSpeed)then
			self.fCurrentSpeed=self.fCurrentSpeed+(self.fDesiredSpeed/100);
			self:SetTimer(0,25);
		else
			self:GotoState("TurnedOn");
		end;
		
	end,
	OnEndState = function( self )

	end,
}

Fan.Server.Decelerating =
{
	OnBeginState = function( self )
		self:SetTimer(0,25);
	end,
	OnTimer = function(self,timerId,msec)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed;
		self:SetAngles(g_Vectors.temp_v1);
		
		if(self.fCurrentSpeed>0.01)then
			self.fCurrentSpeed=self.fCurrentSpeed-(self.fDesiredSpeed/100);
			self:SetTimer(0,25);
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
		self:SetTimer(0,25);
	end,
	OnTimer = function(self,timerId,msec)
		self:GetAngles(g_Vectors.temp_v1);
		g_Vectors.temp_v1.z=g_Vectors.temp_v1.z+self.fCurrentSpeed;
		self:SetAngles(g_Vectors.temp_v1);
		if(self.fCurrentSpeed>0.01)then
			self.fCurrentSpeed=self.fCurrentSpeed-((self.fDesiredSpeed/100)*2);
			self:SetTimer(0,25);
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
