SmartObject = {
  type = "SmartObject",
  Properties = 
  {
		soclasses_SmartObjectClass = "",
--		object_Model = "",
  },
	
	Editor={
	Model="Editor/Objects/anchor.cgf",
	Icon="smartobject.bmp",
	IconOnTop=1,
	},
}

-------------------------------------------------------
--function SmartObject:OnPropertyChange()
--	self:OnReset();
--end

-------------------------------------------------------
function SmartObject:OnInit()
--	self:OnReset();
end

-------------------------------------------------------
function SmartObject:OnReset()
--	if ( System.IsEditor() ) then
--		AI.LogComment("OnReser: System.IsEditor() == true");
--		if ( self.Properties.object_Model == "" ) then
--			do return end;
--		end
--		
--		if ( self.ModelName ~= self.Properties.object_Model ) then
--			self.ModelName = self.Properties.object_Model;
--			-- try to load an animated model first
--			self:LoadObject(0,self.ModelName);
--		--	self:CharacterUpdateOnRender(0,1); -- If it is a character force it to update on render.
--		end
--		
--		-- Enabling drawing of the slot.
--		self:DrawSlot(0,1);
--	else
--		AI.LogComment("OnReser: System.IsEditor() == false");
--		self:LoadObject(0,"");
--		self:DrawSlot(0,0);
--	end;
end

-------------------------------------------------------
function SmartObject:OnUsed()
	-- this function will be called from ACT_USEOBJECT
	BroadcastEvent(self, "Used");
end

-------------------------------------------------------
function SmartObject:OnNavigationStarted(userId)
	self:ActivateOutput("NavigationStarted", userId or NULL_ENTITY)
end

-------------------------------------------------------
function SmartObject:Event_Used( sender )
	BroadcastEvent(self, "Used");
end

SmartObject.FlowEvents =
{
	Inputs =
	{
		Used = { SmartObject.Event_Used, "bool" },
	},
	Outputs =
	{
		Used = "bool",
		NavigationStarted = "entity",
	},
}
