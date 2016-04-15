WaterPuddle = 
{
	type = "WaterPuddle",
	
	Properties = 
	{
		bThisIsAPuddle = 0,
	},
	
	Editor = 
	{
		Model="Editor/Objects/T.cgf",
		Icon="Water.bmp",
		ShowBounds = 1,
		IsScalable = false;
		IsRotatable = true;
	},
}



function WaterPuddle:OnPropertyChange()
end



function WaterPuddle:IsShapeOnly()
	return 0;
end



function WaterPuddle:Event_Hide()
	self:Hide(1);
	self:ActivateOutput("Hidden", true);
end;



function WaterPuddle:Event_UnHide()
	self:Hide(0);
	self:ActivateOutput( "UnHidden", true );
end;




WaterPuddle.FlowEvents =
{
	Inputs =
	{
		Hide = { WaterPuddle.Event_Hide, "bool" },
		UnHide = { WaterPuddle.Event_UnHide, "bool" },
	},
	Outputs =
	{
		Hidden = "bool",
		UnHidden = "bool",
	},
}
