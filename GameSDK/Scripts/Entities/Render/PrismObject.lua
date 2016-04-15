

PrismObject = {
  type = "PrismObject",
	ENTITY_DETAIL_ID = 1,

	Properties = {
	},

	Editor = {
		Model = "Editor/Objects/Particles.cgf",
		Icon = "Clouds.bmp",
	},
}

-------------------------------------------------------
function PrismObject:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

-------------------------------------------------------
function PrismObject:OnInit()
	self:NetPresent(0);
	self:Create();
end


-------------------------------------------------------
function PrismObject:OnShutDown()
end

-------------------------------------------------------
function PrismObject:Create()
	local props = self.Properties;	
	self:LoadPrismObject(0);
end

-------------------------------------------------------
function PrismObject:Delete()
	self:FreeSlot(0);
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function PrismObject:OnReset()
	local bCurrentlyHasObj = self:IsSlotValid(0);
	if (not bCurrentlyHasObj) then
		self:Create();
	end
end


-------------------------------------------------------
function PrismObject:OnSave(tbl)
	tbl.bHasObject = self:IsSlotValid(0);
	if (tbl.bHasObject) then
	  -- todo: save pos
	end
end

-------------------------------------------------------
function PrismObject:OnLoad(tbl)
	local bCurrentlyHasObject = self:IsSlotValid(0);
	local bWantObject = tbl.bHasObject;
	if (bWantObject and not bCurrentlyHasObject) then
		self:Create();
	  -- todo: restore pos
	elseif (not bWantObject and bCurrentlyHasObject) then
		self:Delete();	
	end
end

-------------------------------------------------------
-- Hide Event
-------------------------------------------------------
function PrismObject:Event_Hide()
	self:Delete();
end

-------------------------------------------------------
-- Show Event
-------------------------------------------------------
function PrismObject:Event_Show()
	self:Create();
end

PrismObject.FlowEvents =
{
	Inputs =
	{
		Hide = { PrismObject.Event_Hide, "bool" },
		Show = { PrismObject.Event_Show, "bool" },
	},
	Outputs =
	{
		Hide = "bool",
		Show = "bool",
	},
}
