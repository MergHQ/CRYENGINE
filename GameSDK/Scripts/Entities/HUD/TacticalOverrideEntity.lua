TacticalOverrideEntity = 
{
	Server = {},

	Client = {},
	
	Properties = {
	},
	
	PropertiesInstance = {
	},

	Editor={
		Icon = "Prompt.bmp",
		IconOnTop=1,
	},

	type = "TacticalOverrideEntity",
}

function TacticalOverrideEntity:OnInit()
	self:OnReset();
	
	self:Hide(1);
end

function TacticalOverrideEntity:OnPropertyChange()
	self:OnReset();
end

function TacticalOverrideEntity:OnReset()
end

function TacticalOverrideEntity:OnShutDown()
end

TacticalOverrideEntity.FlowEvents =
{
	Inputs =
	{
	},
	Outputs =
	{
	},
}
