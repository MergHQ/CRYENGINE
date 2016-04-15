BossMarker =
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
	},
}

--------------------------------------------------------------------------
function BossMarker:OnInit()
	self:OnReset();
end

--------------------------------------------------------------------------
function BossMarker:OnPropertyChange()
	self:OnReset();
end

--------------------------------------------------------------------------
function BossMarker:OnReset()
	if(self.Properties.bEnabled == 1)then
		Game.AddTacticalEntity(self.id, eTacticalEntity_Unit);
	else
		Game.RemoveTacticalEntity(self.id, eTacticalEntity_Unit);
	end
end

--------------------------------------------------------------------------
function BossMarker:OnLoad(table)  
	self.Properties.bEnabled = table.enabled;
end

--------------------------------------------------------------------------
function BossMarker:OnSave(table)  
	table.enabled = self.Properties.bEnabled;
end

--------------------------------------------------------------------------
function BossMarker:Event_Enable()
	self.Properties.bEnabled = 1;
	BroadcastEvent( self,"Unhide" );
	Game.AddTacticalEntity(self.id, eTacticalEntity_Unit);
end

--------------------------------------------------------------------------
function BossMarker:Event_Disable()
	self.Properties.bEnabled = 0;
	BroadcastEvent( self,"Hide" );
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Unit);
end

--------------------------------------------------------------------------
BossMarker.FlowEvents =
{
	Inputs =
	{
		Enable = { BossMarker.Event_Enable, "bool" },
		Disable = { BossMarker.Event_Disable, "bool" },
	},
}
