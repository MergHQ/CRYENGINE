--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--
--	Description: AI reinforcement spot
--  
--------------------------------------------------------------------------
--  History:
--  - 7-2-2007  : Created by Mikko
--
--------------------------------------------------------------------------

AIReinforcementSpot = {
	type = "AIReinforcementSpot",
	Properties =
	{
		bEnabled = 1,  -- enabled/disabled
		groupid = 173,
		radius = 30.0,
		AvoidWhenTargetInRadius = 15.0,
		bWhenAllAlerted = 1, -- on alerted / on combat
		bWhenInCombat = 1, -- on alerted / on combat
		iGroupBodyCount = 2,
		eiReinforcementType = 0, -- Default is 0=Wave
	},
}

-------------------------------------------------------
function AIReinforcementSpot:OnInit()
	self:Register();	
end

--------------------------------------------------------------------------
function AIReinforcementSpot:OnPropertyChange()
	self:Register();
end


--------------------------------------------------------------------------
function AIReinforcementSpot:OnReset()
	self.bNowEnabled = self.Properties.bEnabled;
	if (self.Properties.bEnabled == 0) then
		self:TriggerEvent(AIEVENT_DISABLE);
	else
		self:TriggerEvent(AIEVENT_ENABLE);
	end
	self:ActivateOutput("GroupID", self.Properties.groupid);
end

--------------------------------------------------------------------------
function AIReinforcementSpot:Register()
	if (self.Properties.aianchor_AnchorType ~= "") then
		CryAction.RegisterWithAI(self.id, AIAnchorTable.REINFORCEMENT_SPOT, self.Properties);
		-- since sending properties to RegisterWithAI has no effect group id will be set with ChangeParameter
		AI.ChangeParameter(self.id, AIPARAM_GROUPID, self.Properties.groupid);
	end
	self:OnReset();
end

--------------------------------------------------------------------------
function AIReinforcementSpot:Event_Enable(params)
	self:TriggerEvent(AIEVENT_ENABLE);
	self.bNowEnabled = 1;
	self:ActivateOutput("GroupID", self.Properties.groupid);
--	self:ActivateOutput("Enable", true);
end

--------------------------------------------------------------------------
function AIReinforcementSpot:Event_Disable(params)
	self:TriggerEvent(AIEVENT_DISABLE);
	self.bNowEnabled = 0;
--	self:ActivateOutput( "Disable", true);
end

--------------------------------------------------------------------------
function AIReinforcementSpot:Alarm()
--	System.Log("AIReinforcementSpot:Event_Called id="..self.Properties.groupid);
	self:TriggerEvent(AIEVENT_DISABLE);
	self.bNowEnabled = 0;
	self:ActivateOutput("GroupID", self.Properties.groupid);
	self:ActivateOutput("Called", true);
end

--------------------------------------------------------------------------
AIReinforcementSpot.FlowEvents =
{
	Inputs =
	{
		Enable = { AIReinforcementSpot.Event_Enable, "bool" },
		Disable = { AIReinforcementSpot.Event_Disable, "bool" },
	},
	Outputs =
	{
		Called = "bool",
		GroupID = "int",
	},
}
