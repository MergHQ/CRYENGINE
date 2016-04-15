--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2006.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Multifunctional hazard entity.
--  
--------------------------------------------------------------------------
--  History:
--  - 8/2006     : Created by Sascha Gundlach
--
--------------------------------------------------------------------------
-- eiHazardType:
-- 0 = None
-- 1 = Fire
-- 2 = Frost
-- 3 = Electricity
--------------------------------------------------------------------------

Hazard =
{
	Client = {},
	Server = {},
	type = "Trigger",
	Properties=
	{
		bEnabled = 1,
		Damage = {
			fDamage = 50.0,
			eiHazardType = 0,
			bOnlyPlayer = 1,
		},
	},
	Editor=
	{
		Icon = "hazard.bmp",
		Model="Editor/Objects/T.cgf",
		ShowBounds = 1,
	},
	States = {"Activated","Deactivated","Turning"},
}

Net.Expose {
	Class = Hazard,
	ClientMethods = {
		AddScreenEffect 			= {RELIABLE_UNORDERED, PRE_ATTACH, ENTITYID },
		RemoveScreenEffect 		= {RELIABLE_UNORDERED, PRE_ATTACH, ENTITYID },
		DoFX							 		= {RELIABLE_UNORDERED, PRE_ATTACH, },
		InitFX 								= {RELIABLE_UNORDERED, PRE_ATTACH },
	},
	
	ServerMethods = {
	},
	ServerProperties = {
	}
}

CHECK_TIMER = 0;
FX_TIMER = 1;

function Hazard:OnReset()
	self.ents={};
 	local props=self.Properties;
	self:InitFX();
	if(props.bEnabled==1)then
		Game.AddTacticalEntity(self.id, eTacticalEntity_Hazard);
		self:GotoState("Activated");
	else
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_Hazard);
		self:GotoState("Deactivated");
	end;

	self:RegisterForAreaEvents(self.Properties.bEnabled);

end;

function Hazard:OnSave(tbl)
	tbl.ents=self.ents;
	tbl.fx=self.fx;
end;

function Hazard:OnLoad(tbl)
	self.ents=tbl.ents;
	self.fx=tbl.fx;
end;

function Hazard:OnPropertyChange()
	self:OnReset();
end;

function Hazard.Client:OnInit()
	--self.ents={};
	self:OnReset();
end;

function Hazard.Server:OnInit()
	self:OnReset();
end;


function Hazard.Client:InitFX()
	self:InitFX();
end;

function Hazard.Client:AddScreenEffect(entId)
	local ent=System.GetEntity(entId);
	if(ent)then
		self:AddScreenEffect(ent);
	end
end;

function Hazard.Client:RemoveScreenEffect(entId)
	local ent=System.GetEntity(entId);
	if (ent) then
		self:RemoveScreenEffect(ent);
	end
end;

function Hazard.Client:DoFX()
	self:DoFX();
end;


function Hazard:InitFX()
	self.fx={
		none,
		fire={
			active=0,
			desired=0,
			current=0,
		},
		frost={
			active=0,
			desired=0,
			current=0,
		},
		electricity={
			active=0,
			desired=0,
			current=0,
		},
	};
	local tmp=self.Properties.Damage.eiHazardType;
	self.dmg="";
	if(tmp==0)then
		self.dmg="none";
	elseif(tmp==1)then
		self.dmg="fire";
	elseif(tmp==2)then
		self.dmg="frost";
	elseif(tmp==3)then
		self.dmg="electricity";
	else
		return;
	end;
	
end;

function Hazard:HandleEntity(ent)
	local props=self.Properties.Damage;
	local tmp={x=0,y=0,z=0};
	local dist;
	
	if(ent~=nil and ent.actor~=nil)then
		if(self.Properties.Damage.bOnlyPlayer==1 and not ent.actor:IsPlayer())then return;end;
		local dmg=self.Properties.Damage.fDamage;
				
		if(dmg>0)then
			g_gameRules:CreateHit(ent.id,self.id,self.id,dmg,nil,nil,nil,"fire");
		end;
	end;
end

function Hazard:HandleEntities()
	for i,v in pairs(self.ents) do
		self:HandleEntity(v);
	end;
end;

function Hazard:AddScreenEffect(ent)
	if(ent and ent.actor and ent.actor:IsPlayer())then
		local dmg=self.Properties.Damage.eiHazardType;
		if(dmg==1)then
			if(self.fx.fire.active~=1)then
				self.fx.fire.active=1;
				self.fx.fire.desired=1;
				self.fx.fire.current=0;
				System.SetScreenFx("FilterBlurring_Type", 0);
				System.SetScreenFx("FilterBlurring_Amount", 0);
				self:SetTimer(FX_TIMER,25);
			end;
		elseif(dmg==2)then
			if(self.fx.frost.active~=1)then
				self.fx.frost.active=1;
				self.fx.frost.desired=1;
				self.fx.frost.current=0;
				System.SetScreenFx("ScreenFrost_Active", 1);
				System.SetScreenFx("ScreenFrost_CenterAmount", 1);
				System.SetScreenFx("ScreenFrost_Amount", 0);
				self:SetTimer(FX_TIMER,25);
			end;
		elseif(dmg==3)then
			if(self.fx.electricity.active~=1)then
				self.fx.electricity.active=1;
				self.fx.electricity.desired=2;
				self.fx.electricity.current=0;
				System.SetScreenFx("AlienInterference_Active", 1);
				System.SetScreenFx("AlienInterference_Amount", 1);
				self:SetTimer(FX_TIMER,25);
			end;
		end;
	end;
end;

function Hazard:RemoveScreenEffect(ent)
	if(ent and ent.actor and ent.actor:IsPlayer())then
		local dmg=self.dmg;
		if(self.fx[dmg])then
			if(self.fx[dmg].active==1)then
				self.fx[dmg].active=0;
				self.fx[dmg].desired=0;
				self:SetTimer(FX_TIMER,25);
			end;
		end;
	end;
end;

function Hazard:DoFX()
	local dmg=self.dmg;
	if(dmg~="none")then
		if(self.fx[dmg].current<0.01 and self.fx[dmg].desired==0)then
			self.fx[dmg].current=0;
			if(dmg=="frost")then
				System.SetScreenFx("ScreenFrost_Amount",self.fx[dmg].current);
				System.SetScreenFx("ScreenFrost_Active", 0);
			elseif(dmg=="fire")then
				System.SetScreenFx("FilterBlurring_Amount",0);
			elseif(dmg=="fire")then
				System.SetScreenFx("AlienInterference_Amount",0);
				System.SetScreenFx("AlienInterference_Active",0);
			end;
		elseif(self.fx[dmg].current<self.fx[dmg].desired)then
			if(dmg=="frost")then
				self.fx[dmg].current=self.fx[dmg].current+0.01;
				System.SetScreenFx("ScreenFrost_Amount",self.fx[dmg].current);
			elseif(dmg=="fire")then
				self.fx[dmg].current=self.fx[dmg].current+0.01;
				System.SetScreenFx("FilterBlurring_Amount",self.fx[dmg].current);
			elseif(dmg=="electricity")then
				self.fx[dmg].current=self.fx[dmg].current+0.1;
				System.SetScreenFx("AlienInterference_Amount",self.fx[dmg].current);
			end;
			self:SetTimer(FX_TIMER,25);
		elseif(self.fx[dmg].current>self.fx[dmg].desired)then
			if(dmg=="frost")then
				self.fx[dmg].current=self.fx[dmg].current-0.01;
				System.SetScreenFx("ScreenFrost_Amount",self.fx[dmg].current);
			elseif(dmg=="fire")then
				self.fx[dmg].current=self.fx[dmg].current-0.03;
				System.SetScreenFx("FilterBlurring_Amount",self.fx[dmg].current);
			elseif(dmg=="electricity")then
				self.fx[dmg].current=self.fx[dmg].current-0.1;
				System.SetScreenFx("AlienInterference_Amount",self.fx[dmg].current);
			end;
			self:SetTimer(FX_TIMER,25);
		end;
	end;
end;
-----------------------------------------------------------------------------------
function Hazard:Event_Activated()
	Game.AddTacticalEntity(self.id, eTacticalEntity_Hazard);
	self:GotoState("Activated");
	self:ActivateOutput("Activated",true);
	self:RegisterForAreaEvents(1);
end;

function Hazard:Event_Deactivated()
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Hazard);
	self:GotoState("Deactivated");
	self:ActivateOutput("Deactivated",true);
	self:RegisterForAreaEvents(0);
end;

-----------------------------------------------------------------------------------
-- need to check this in the OnEnterArea functions, because if not, entities can be duplicated after loading a savegame. (as soon as the entity moves, it triggers again the OnEnterArea even if it was already inside)
-- TODO: it probably should be fixed in engine side, but this is faster and safer at this point (near crysis2 end)
function Hazard:IsEntityAlreadyInTable( entity )
	for i,v in pairs(self.ents) do
		if(v.id==entity.id) then
		  return true;
		end
  end
  return false;
end

-----------------------------------------------------------------------------------
Hazard.Server.Activated=
{
	OnBeginState = function(self)
		if(table.getn(self.ents)>0)then
			self:SetTimer(CHECK_TIMER,1000);
			for i,v in pairs(self.ents) do
				if(v.actor and v.actor:IsPlayer())then
					self.onClient:AddScreenEffect(v.actor:GetChannel(),v.id);
				end;
			end;
		end;
	end,
	OnEnterArea = function(self,entity,areaId)
			if (not self:IsEntityAlreadyInTable( entity )) then
				self:ActivateOutput("Enter",true);
				table.insert(self.ents,entity);
				self:SetTimer(CHECK_TIMER,1000);
				-- give initial damage
				self:HandleEntity(entity);
				if(entity.actor and entity.actor:IsPlayer())then
					self.onClient:AddScreenEffect(entity.actor:GetChannel(), entity.id);
				end;
			end
	end,
	OnLeaveArea = function(self,entity,areaId)
		for i,v in pairs(self.ents) do
			if(v==entity)then
				self:ActivateOutput("Leave",true);
				table.remove(self.ents,i);
				if(v.actor and v.actor:IsPlayer())then
					self.onClient:RemoveScreenEffect(v.actor:GetChannel(), v.id);
				end;
				break;
			end;
		end;
	end,
	OnTimer = function(self,timerId,msec)
		if(timerId==CHECK_TIMER)then
			if(self.ents and table.getn(self.ents)>0 )then
				self:HandleEntities();
				self:SetTimer(CHECK_TIMER,1000);
			end;
		end;
	end,
	OnEndState = function(self)

	end,
};


Hazard.Client.Activated=
{
	OnTimer = function(self,timerId,msec)
		if(timerId==FX_TIMER)then
			self:DoFX();
		end;
	end
}

Hazard.Client.Deactivated=
{
	OnTimer = function(self,timerId,msec)
		if(timerId==FX_TIMER)then
			self:DoFX();
		end;
	end
}

Hazard.Server.Deactivated=
{
	OnBeginState = function(self)
		for i,v in pairs(self.ents) do
			if(v.actor and v.actor:IsPlayer())then
				self.onClient:RemoveScreenEffect(v.actor:GetChannel(), v.id);
			end;
		end;
	end,
	OnEnterArea = function(self,entity,areaId)
		if (not self:IsEntityAlreadyInTable( entity )) then
			table.insert(self.ents,entity);
		end
	end,
	OnLeaveArea = function(self,entity,areaId)
		for i,v in pairs(self.ents) do
			if(v==entity)then
				table.remove(self.ents,i);
				self:RemoveScreenEffect(v);
				break;
			end;
		end;
	end,

};

-----------------------------------------------------------------------------------

Hazard.FlowEvents =
{
	Inputs =
	{
		Activated = { Hazard.Event_Activated, "bool" },
		Deactivated = { Hazard.Event_Deactivated, "bool" },
	},
	Outputs =
	{
		Activated = "bool",
		Deactivated = "bool",
		Enter = "bool",
		Leave = "bool",
	},
}