--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--
--	Description: BasicAIEvents - moved out all the events from BasicAI
--	
--  
--------------------------------------------------------------------------
--  History:
--  - 13/06/2005   15:36 : created by Kirill Bulatsev
--
--------------------------------------------------------------------------


BasicAIEvent =
{

}

MakeUsable(BasicAIEvent);


--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_WakeUp( params )
	if (not AI.IsEnabled(self.id)) then
		return
	end

	if (self.actor:GetPhysicalizationProfile() == "sleep") then
		self.actor:StandUp();
	end
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Sleep( params )
--System.Log(">>>>>>> BasicAIEvent:Event_Sleep "..self:GetName());
	if(not self.isFallen) then
		BroadcastEvent(self, "Sleep");
	end	
	self.actor:Fall(self:GetPos());
	self.isFallen = 1;
end


--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Enabled( params )
	BroadcastEvent(self, "Enabled");
end

--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Disabled( params )
	BroadcastEvent(self, "Disabled");
end


--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_OnAlert( params )
	BroadcastEvent( self, "OnAlert" );
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Disable(params)
	--hide does enable/disable physics as well
	self:Hide(1)
	--self:EnablePhysics(0);
	self:TriggerEvent(AIEVENT_DISABLE);
	--AI.LogEvent(" >>> BasicAI:Event_Disable  "..self:GetName());
	
	self._enabled=false;
	
	self:Event_Disabled(self);
	
	if (CryAction.IsServer()) then
		self.allClients:ClAIDisable();
	end

	GameAI.PauseAllModules(self.id);
	
	if (self.OnDisabled) then
		self:OnDisabled()
	end
end
 
--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Enable(params)
	if (not self:IsDead() ) then 
		self._enabled=true;
		-- hide does enable/disable physics as well
		self:Hide(0)
		GameAI.ResumeAllModules(self.id);
		self:Event_Enabled(self);
		
		if (self.SoundSignals and self.SoundSignals.Idle) then
			GameAudio.PlayEntitySignal(self.SoundSignals.Idle, self.id)
		end
		
		if (CryAction.IsServer()) then
			self.allClients:ClAIEnable();
		end
	end
	
	if (self.OnEnabled) then
		self:OnEnabled()
	end
end


--
--	This event will kill the actor 
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Kill(params)
	if (not AI.IsEnabled(self.id)) then
		return
	end

	if (not self:IsDead()) then 
		g_gameRules:CreateHit(self.id,self.id,self.id,100000,nil,nil,nil,"event");
	end
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_SetForcedLookDir(sender,params)
	
	self.actor:SetForcedLookDir(params);
	
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_ClearForcedLookDir(params)
	
	self.actor:ClearForcedLookDir();
	
end


--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_SetForcedLookObjectId(sender, params)
	self.actor:SetForcedLookObjectId(params.id);
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_ClearForcedLookObjectId(params)
	self.actor:ClearForcedLookObjectId();
end


---------------------------------------------------------------------------------------------------------
--
--
--			below are old events - to-be-removed candidates
--
---------------------------------------------------------------------------------------------------------
--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Follow(sender)
	BroadcastEvent(self, "Follow");
	local newGroupId;
	if(sender.Who and sender.Who.id and sender.Who.id~=NULL_ENTITY) then -- it's a trigger
		newGroupId = AI.GetGroupOf(sender.Who.id);
	else
		newGroupId = AI.GetGroupOf(sender.id);
	end
	AI.ChangeParameter(self.id,AIPARAM_GROUPID,newGroupId);
	AI.Signal(SIGNALFILTER_SENDER,0,"FOLLOW_LEADER",self.id);
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_Test(sender)
--
--	AI.SetPFProperties(self.id, AIPATH_HUMAN);
--do return end
--	self:StartAnimation(0,"swim_idle_nw_01",4,.1);
--do return end
--
--	local point = System.GetEntityByName("place");
--	if (point) then 
--		g_SignalData.point = point:GetWorldPos();
--		AI.Signal(SIGNALFILTER_SENDER, 0, "ORDER_MOVE", self.id, g_SignalData);
--	end
--		
	g_SignalData.fValue = 2;
	AI.Signal(SIGNALFILTER_LEADER,0,"OnScaleFormation",self.id,g_SignalData);
end

--
--
--------------------------------------------------------------------------------------------------------
function BasicAIEvent:Event_TestStealth(sender)
	AI.SetPFProperties(self.id, AIPATH_HUMAN_COVER);
end


--function BasicAIEvent:Event_AlertStatus(sender)
--	AI.LogEvent("ALERT STATUS CHANGING TO "..sender.Properties.ReferenceName);
--	AI.Signal(SIGNALFILTER_LEADER, 0, "OnAlertStatus_"..sender.Properties.ReferenceName, self.id);
--end

function BasicAIEvent:Event_ResetHealth()
	entity.actor:SetHealth(entity.actor:GetMaxHealth());
	self.lastHealth = self.actor:GetHealth();
end

function BasicAIEvent:Event_MakeVulnerable()
	self.AI.invulnerable = false;
end

function BasicAIEvent:Event_MakeInvulnerable()
	self.AI.invulnerable = true;
end


BasicAIEvent.FlowEvents =
{
	Inputs =
	{
		Used = { BasicAIEvent.Event_Used, "bool" },
		EnableUsable = { BasicAIEvent.Event_EnableUsable, "bool" },
		DisableUsable = { BasicAIEvent.Event_DisableUsable, "bool" },
		
		Disable = { BasicAIEvent.Event_Disable, "bool" },
		Enable = { BasicAIEvent.Event_Enable, "bool" },
		Kill = { BasicAIEvent.Event_Kill, "bool" },
		Sleep = { BasicAIEvent.Event_Sleep, "bool" },
		WakeUp = { BasicAIEvent.Event_WakeUp, "bool" },		-- fall-and-play stand up
		ResetHealth = { BasicAIEvent.Event_ResetHealth, "any" };
		MakeVulnerable = { BasicAIEvent.Event_MakeVulnerable, "any" };
		MakeInvulnerable = { BasicAIEvent.Event_MakeInvulnerable, "any" };
		
		SetForcedLookDir = { BasicAIEvent.Event_SetForcedLookDir, "Vec3" },
		ClearForcedLookDir = { BasicAIEvent.Event_ClearForcedLookDir, "bool" },
		
		SetForcedLookObjectId = { BasicAIEvent.Event_SetForcedLookObjectId, "entity" },
		ClearForcedLookObjectId = { BasicAIEvent.Event_ClearForcedLookObjectId, "bool" },
	},
	Outputs =
	{
		Used = "bool",

		Dead = "bool",
		OnAlert = "bool",
		Sleep = "bool",				-- fall-and-play falling
--		Awake = "bool",
		
		Enabled = "bool",
		Disabled = "bool",
	},
}
