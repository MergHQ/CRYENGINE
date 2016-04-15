MapIconEntity =
{
	Editor =
	{
		Icon="Comment.bmp",
	},
	Properties =
	{
		fileModel				= "",
		ModelSubObject	= "",
		bEnabled				= 1,
		sMapIcon				= "",
	},
}

--------------------------------------------------------------------------
function MapIconEntity:OnInit()
	self:OnReset();
end

--------------------------------------------------------------------------
function MapIconEntity:OnPropertyChange()
	self:OnReset();
end

--------------------------------------------------------------------------
function MapIconEntity:OnReset()
	if(self.Properties.bEnabled == 1)then
		Game.AddTacticalEntity(self.id, eTacticalEntity_MapIcon);
	else
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_MapIcon);
	end
end

--------------------------------------------------------------------------
function MapIconEntity:OnLoad(table)  
	self.Properties.bEnabled = table.enabled;
end

--------------------------------------------------------------------------
function MapIconEntity:OnSave(table)  
	table.enabled = self.Properties.bEnabled;
end

--------------------------------------------------------------------------
function MapIconEntity:Event_Enable()
	self.Properties.bEnabled = 1;
	Game.AddTacticalEntity(self.id, eTacticalEntity_MapIcon);
end

--------------------------------------------------------------------------
function MapIconEntity:Event_Disable()
	self.Properties.bEnabled = 0;
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_MapIcon);
end

--------------------------------------------------------------------------
MapIconEntity.FlowEvents =
{
	Inputs =
	{
		Enable = { MapIconEntity.Event_Enable, "bool" },
		Disable = { MapIconEntity.Event_Disable, "bool" },
	},
}
