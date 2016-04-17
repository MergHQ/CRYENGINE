--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Mission Objective
--
--------------------------------------------------------------------------
--  History:
--  - 19:4:2005   12:00 : Created by Filippo De Luca
--  - 13:2:2006   12:00 : Refactoring by AlexL
--------------------------------------------------------------------------

MissionObjective =
{
	Properties =
	{
		mission_MissionID = "", -- Mission objective ID
		bShowOnRadar = 1, -- Show Objective on Radar
		sTrackedEntityName = "", -- Track this entity instead of the MissionObjective entity itself
	},

	Editor =
	{
		Model="Editor/Objects/T.cgf",
		Icon="TagPoint.bmp",
	},

	States = { "Deactivated", "Activated", "Failed", "Completed" },
}

----------------------------------------------------------------------------------------------------
function MissionObjective:OnPropertyChange()
	self:Reset();
end

----------------------------------------------------------------------------------------------------
function MissionObjective:OnReset()
	self:Reset();
end


----------------------------------------------------------------------------------------------------
function MissionObjective:OnSpawn()
	self:Reset();
end


----------------------------------------------------------------------------------------------------
function MissionObjective:OnDestroy()
end


----------------------------------------------------------------------------------------------------
function MissionObjective:Reset()
	if (self.Properties.mission_MissionID ~= "") then
		self.missionID = self.Properties.mission_MissionID;
	else
		self.missionID = self:GetName();
	end

	self.silent = false;
	self.noFG = true; -- prevent output on Deactivate
	self:GotoState("Deactivated");
	self.noFG = false;
  -- self:ActivateOutput( "Deactivated", true );
end

-------------------------------------------------------------------------------
-- Deactivated State
-------------------------------------------------------------------------------
MissionObjective.Deactivated =
{
	OnBeginState = function( self )
		System.Log("MissionObjective "..self.missionID.." Deactivated");
		if (HUD) then
			HUD.SetObjectiveStatus(self.missionID, MO_DEACTIVATED, self.silent);
		end
		if (self.noFG == false) then
  	  self:ActivateOutput( "Deactivated", true );
  	end
	end
}

-------------------------------------------------------------------------------
-- Activated State
-------------------------------------------------------------------------------
MissionObjective.Activated =
{
	OnBeginState = function( self )
		System.Log("MissionObjective "..self.missionID.." Activated");
		if (HUD) then
			if (self.Properties.bShowOnRadar ~= 0) then
				local target = self:GetName();
				if (self.Properties.sTrackedEntityName ~= "") then
					target = self.Properties.sTrackedEntityName;
				end
				if (target and target ~= "") then
					HUD.SetObjectiveEntity(self.missionID, target);
				end
			else
				HUD.ClearObjectiveEntity(self.missionID);
			end
			HUD.SetObjectiveStatus(self.missionID, MO_ACTIVATED, self.silent);
		end
	  self:ActivateOutput( "Activated", true );
	end,

	OnEndState = function ( self )
	end
}

-------------------------------------------------------------------------------
-- Failed State
-------------------------------------------------------------------------------
MissionObjective.Failed =
{
	OnBeginState = function( self )
		System.Log("MissionObjective "..self.missionID.." Failed");
		if (HUD) then
			HUD.SetObjectiveStatus(self.missionID, MO_FAILED, false);
		end
	  self:ActivateOutput( "Failed", true );
	end
}

-------------------------------------------------------------------------------
-- Completed State
-------------------------------------------------------------------------------
MissionObjective.Completed =
{
	OnBeginState = function( self )
		System.Log("MissionObjective "..self.missionID.." Completed");
		if (HUD) then
			HUD.SetObjectiveStatus(self.missionID, MO_COMPLETED, false);
		end
	  self:ActivateOutput( "Completed", true );
	end
}

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_Deactivate(sender)
	self.silent = false;
	self:GotoState( "Deactivated" );
end

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_Activate(sender)
	self.silent = false;
	self:GotoState( "Activated" );
end

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_Failed(sender)
	self.silent = false;
	self:GotoState( "Failed" );
end

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_Completed(sender)
	if (self:IsInState( "Activated" ) ~= 0)  then
		self.silent = false;
		self:GotoState( "Completed" );
	end
end

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_SilentDeactivate(sender)
	self.silent = true;
	self:GotoState( "Deactivated" );
end

----------------------------------------------------------------------------------------------------
function MissionObjective:Event_SilentActivate(sender)
	self.silent = true;
	self:GotoState( "Activated" );
end


MissionObjective.FlowEvents =
{
	Inputs =
	{
		Deactivate = { MissionObjective.Event_Deactivate, "bool" },
		Activate   = { MissionObjective.Event_Activate, "bool" },
		Failed     = { MissionObjective.Event_Failed, "bool" },
		Completed  = { MissionObjective.Event_Completed, "bool" },
		SilentDeactivate = { MissionObjective.Event_SilentDeactivate, "bool" },
		SilentActivate   = { MissionObjective.Event_SilentActivate, "bool" },
	},

	Outputs =
	{
		Deactivated = "bool",
		Activated   = "bool",
		Failed      = "bool",
		Completed   = "bool",
	},
}
