----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Power struggle capture node (Power struggle gamemode)
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 22:03:2011  : Created by Colin Gulliver
--
----------------------------------------------------------------------------------------------------

Script.ReloadScript("Scripts/Entities/AlienTech/AlienTech.lua");

PowerStruggleNode = new (AlienTech);

PowerStruggleNode.Properties.ControlRadius = 8;
PowerStruggleNode.Properties.ControlHeight = 5;
PowerStruggleNode.Properties.ControlOffsetZ = 0;
PowerStruggleNode.Properties.DebugDraw = 0;
PowerStruggleNode.Properties.Letter = "";
PowerStruggleNode.NumTimesEmerged = 0;

PowerStruggleNode.UseableChildEntity = 0

----------------------------------------------------------------------------------------------------
function PowerStruggleNode.Server:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode.Client:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:OnReset()
	local radius = self.Properties.ControlRadius;
	local offsetZ = self.Properties.ControlOffsetZ;
	local height = self.Properties.ControlHeight;

	local min = { x=-radius, y=-radius, z=offsetZ };
	local max = { x=radius, y=radius, z=(height + offsetZ) };

	self:SetTriggerBBox( min, max );
	
	self:SetViewDistRatio(255);

	-- for debugging in game remove this IsEditor check
	if (System.IsEditor()) then
		if (self.Properties.DebugDraw ~= 0) then
			-- get OnUpdate() being called so we can debug draw our shapes
			self:Activate(1);
			CryAction.ForceGameObjectUpdate(self.id, true);
		else
			self:Activate(0);
		end
	end
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:OnPropertyChange()
	self:OnReset();
end


function PowerStruggleNode.Server:OnUpdate()
	-- for debugging in game remove this IsEditor check
	if (System.IsEditor()) then
		if (self.Properties.DebugDraw ~= 0) then
			local pos = self:GetWorldPos();
			local radius = self.Properties.ControlRadius;
			local height = self.Properties.ControlHeight;
			local offsetZ = self.Properties.ControlOffsetZ;

			--pos.z = pos.z + (height * 0.5) + offsetZ;
			pos.z = pos.z + offsetZ;

			Game.DebugDrawCone(pos.x, pos.y, pos.z, radius, height, 60, 60, 255, 150);
		end
	end
end


----------------------------------------------------------------------------------------------------
function PowerStruggleNode:ActivateNode()
	Log("PowerStruggleNode:ActivateNode()");
	
	BroadcastEvent(self, "Activated");

	self:SetViewDistRatio(255);	-- try to ensure that the nodes are always visible
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:DeActivateNode()
		BroadcastEvent(self, "DeActivatedImmediately"); -- broadcast DeActivated immediately to play the down anim idle directly
		self:Event_Buried();
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:Event_Buried()
	Log("PowerStruggleNode:Event_Buried()");
	g_gameRules.game:SendGameRulesObjectiveEntitySignal(self.id, 0); -- signal=CGameRulesObjective_PowerStruggle::eEntitySignal_node_buried==0
	
	-- self:Hide(1); -- these events cannot be relied on, on clients
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:Event_Emerging()
	Log("PowerStruggleNoed:Event_Emerging()");
	
	if (self.NumTimesEmerged == 0) then
		BroadcastEvent(self, "OnFirstEmerging");
	else
		BroadcastEvent(self, "OnRepeatEmerging");
	end
	BroadcastEvent(self, "OnAnyEmerging");
	
	self.NumTimesEmerged = self.NumTimesEmerged+1;

	-- self:Hide(0); -- these events cannot be relied on, on clients
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:Event_Emerged()
	Log("PowerStruggleNode:Event_Emerged()");
	g_gameRules.game:SendGameRulesObjectiveEntitySignal(self.id, 1); -- signal=CGameRulesObjective_PowerStruggle::eEntitySignal_node_emerged==1
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:Event_Burying()
	Log("PowerStruggleNoed:Event_Burying()");
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:Event_ShowIcon()
	Log("PowerStruggleNoed:Event_ShowIcon()");
end

----------------------------------------------------------------------------------------------------
function PowerStruggleNode:IsUsable(user)
	self.UseableChildEntity = g_gameRules.game:CanUsePowerStruggleNode(user.id, self.id)
	return self.UseableChildEntity
end

----------------------------------------------------------------------------------------------------
PowerStruggleNode.FlowEvents =
{
	Inputs =
	{
		Buried = { PowerStruggleNode.Event_Buried, "any" },
		Emerging = { PowerStruggleNode.Event_Emerging, "any" },
		Emerged = { PowerStruggleNode.Event_Emerged, "any" },
		Burying = { PowerStruggleNode.Event_Burying, "any" },
		ShowIcon = { CrashSiteLocation.Event_ShowIcon, "any" },
	},
	Outputs =
	{
		Activated = "bool",
		OnFirstEmerging = "bool",
		OnRepeatEmerging = "bool",
		OnAnyEmerging = "bool",
		DeActivated = "bool",
		DeActivatedImmediately = "bool",
	},
}