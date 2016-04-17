Script.ReloadScript("scripts/Utils/EntityUtils.lua")

-- Basic entity
EntityFlashTag =
{
	Properties =
	{
		soclasses_SmartObjectClass = "",
		bAutoGenAIHidePts = 0,

		object_Model = "Objects/default/primitive_plane_small.cgf",
		fScale = 2.0, -- Set the scale of the tag to appear above players heads
	},

	Client = {},
	Server = {},

	-- Temp.
	_Flags = {},

	Editor =
	{
		Icon = "physicsobject.bmp",
		IconOnTop = 1,
	},

}
------------------------------------------------------------------------------------------------------
function EntityFlashTag:OnSpawn()
	self:SetFromProperties();
end

------------------------------------------------------------------------------------------------------
function EntityFlashTag:SetFromProperties()
	local Properties = self.Properties;

	if (Properties.object_Model == "") then
		do return end;
	end

	self:LoadObject(0,Properties.object_Model);
	self:SetScale(Properties.fScale);

end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function EntityFlashTag:OnPropertyChange()
	self:SetFromProperties();
end
