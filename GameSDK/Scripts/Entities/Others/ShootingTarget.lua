--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2006.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Shooting target.
--  
--------------------------------------------------------------------------
--  History:
--  - 8/2006     : Created by Sascha Gundlach
--
--------------------------------------------------------------------------

ShootingTarget =
{
	Client = {},
	Server = {},
	Properties=
	{
		fileModel 					= "objects/library/architecture/aircraftcarrier/props/misc/target.cgf",
		bTurningMode = 1,
		fIntervallMin = 3,
		fIntervallMax = 5,
		fLightUpTime = 2,
		fTurnSpeed = 0.5,
		soclasses_SmartObjectClass = "ShootingTarget",
	},
	Editor=
	{
		Icon="Item.bmp",
		ShowBounds = 1,
	},
	States = {"Activated","Deactivated","Turning","Init"},
}
ACTIVATION_TIMER=0;
TURN_TIMER=1;

function ShootingTarget:OnReset()
 	local props=self.Properties;
 	if(not EmptyString(props.fileModel))then
 		self:LoadObject(0,props.fileModel);
 	end;
 	EntityCommon.PhysicalizeRigid(self,0,self.physics,0);
 	self.side=0;
 	self.ended=0;
 	self:GetAngles(self.initialrot);
 	CopyVector(self.turnrot,self.initialrot);
 	self.turnrot.z=self.turnrot.z+(math.pi/2);
 	self:Activate(1);
 	self:GotoState("Deactivated");
end;

function ShootingTarget:OnSave(tbl)
	tbl.side=self.side;
	tbl.ended=self.ended;
end;

function ShootingTarget:OnLoad(tbl)
	self.side=tbl.side
	self.ended=tbl.ended;
end;

function ShootingTarget:OnPropertyChange()
	self:OnReset();
end;

function ShootingTarget.Server:OnInit()
	self.physics = {
		bRigidBody=0,
		bRigidBodyActive = 0,
		Density = -1,
		Mass = -1,
	};
	self.tmp={x=0,y=0,z=0};
	self.turnrot={x=0,y=0,z=0};
	self.initialrot={x=0,y=0,z=0};
	self.inc=0;
	self.init=1;
	self:OnReset();
end;

function ShootingTarget.Server:OnHit(hit)
	if(self:GetState()~="Activated")then return;end;
	local vTmp=g_Vectors.temp;
	SubVectors(vTmp,self:GetPos(),hit.pos);
	local dist=LengthVector(vTmp);
	dist=((1-(dist*2))+0.08)*10;
	if(dist>9.4)then
		dist=10;
	else
		dec=string.find(dist,".",1,1)
		dist=tonumber(string.sub(dist,1,dec-1))
	end;
	
	if(self.Properties.bTurningMode==1)then
		local shooter=0;
		if(hit.shooter and hit.shooter==g_localActor)then
			shooter=1;
		else
			shooter=2;
		end;
		if(self.side==1)then
			self:GotoState("Init");
			self:ActivateOutput("Hit",dist);
		else
			self:ActivateOutput("Hit",-1);
		end;
	else
		--Normal mode just outputs hit
	end;
end;

-----------------------------------------------------------------------------------
function ShootingTarget:Event_Activated()
	self:GotoState("Init");
	BroadcastEvent(self, "Activated")
end;

function ShootingTarget:Event_Deactivated()
	self.ended=1;
	self:GotoState("Deactivated");
	BroadcastEvent(self, "Deactivated")
end;

-----------------------------------------------------------------------------------
ShootingTarget.Server.Deactivated=
{
	OnBeginState = function(self)
		if(self.init==0)then
			local props=self.Properties;
			self:KillTimer(ACTIVATION_TIMER);
			self:KillTimer(TURN_TIMER);
			AI.SetSmartObjectState(self.id,"Neutral");
			if(props.fIntervallMin>props.fIntervallMax)then props.fIntervallMin=props.fIntervallMax;end;
			self:SetTimer(ACTIVATION_TIMER,20);
		else
			self.init=0;
		end;
	end,
	OnTimer = function(self,timerId,msec)
		if(timerId==ACTIVATION_TIMER)then
			self:GetAngles(self.tmp);
			self.inc=self.inc+self.Properties.fTurnSpeed;
			self.tmp.z=self.tmp.z+self.Properties.fTurnSpeed;
			self:SetAngles(self.tmp);
			if(self.inc<math.pi)then
				self:SetTimer(ACTIVATION_TIMER,20);
			else
				self:SetAngles(self.initialrot);
			end;
		end;
	end,
};

ShootingTarget.Server.Activated=
{
	OnBeginState = function(self)
		self.ended=0;
		if(self.Properties.bTurningMode==1)then
			AI.SetSmartObjectState(self.id,"Neutral");
			local props=self.Properties;
			if(props.fIntervallMin>props.fIntervallMax)then props.fIntervallMin=props.fIntervallMax;end;
			self:SetTimer(ACTIVATION_TIMER,1000*random(props.fIntervallMin,props.fIntervallMax));
		end;
	end,
	OnTimer = function(self,timerId,msec)
		if(self.ended==1)then
			self:GotoState("Deactivated");
			return;
		end;
		if(timerId==ACTIVATION_TIMER)then
			self:GetAngles(self.tmp);
			self.inc=self.inc+self.Properties.fTurnSpeed;
			self.tmp.z=self.tmp.z-self.Properties.fTurnSpeed;
			self:SetAngles(self.tmp);
			if(self.inc<math.pi)then
				self:SetTimer(ACTIVATION_TIMER,20);
			else
				self:SetAngles(self.initialrot);
				AI.SetSmartObjectState(self.id,"Colored");
				self.side=1;
				self:SetTimer(TURN_TIMER,1000*self.Properties.fLightUpTime);
			end;
		elseif(timerId==TURN_TIMER)then
			AI.SetSmartObjectState(self.id,"Neutral");
			self:GotoState("Init");
		end;
	end,
};

ShootingTarget.Server.Init=
{
	OnBeginState = function(self)
		self:KillTimer(ACTIVATION_TIMER);
		self:KillTimer(TURN_TIMER);
		self.inc=0;
		self.side=0;
		self:SetTimer(ACTIVATION_TIMER,25);
	end,
	OnTimer = function(self,timerId,msec)
		if(timerId==ACTIVATION_TIMER)then
			self:GetAngles(self.tmp);
			self.inc=self.inc+self.Properties.fTurnSpeed;
			self.tmp.z=self.tmp.z+self.Properties.fTurnSpeed;
			self:SetAngles(self.tmp);
			if(self.inc<math.pi/2)then
				self:SetTimer(ACTIVATION_TIMER,25);
			else
				self:SetAngles(self.turnrot);
				self:GotoState("Activated");
			end;
		end;
	end,
};

-----------------------------------------------------------------------------------

ShootingTarget.FlowEvents =
{
	Inputs =
	{
		Deactivated = { ShootingTarget.Event_Deactivated, "bool" },
		Activated = { ShootingTarget.Event_Activated, "bool" },
	},
	Outputs =
	{
		Deactivated = "bool",
		Activated = "bool",
		Hit = "int",
		PlayerOne = "int",
		PlayerTwo = "int",
	},
}