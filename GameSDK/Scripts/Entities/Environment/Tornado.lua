Tornado = {
	type = "Tornado",
	Properties = {
		Radius = 30.0,
		fWanderSpeed = 10.0,
		FunnelEffect = "weather.tornado.large",
		fCloudHeight = 376.0,
		
		fSpinImpulse = 9.0,
		fAttractionImpulse = 13.0,
		fUpImpulse = 18.0,
	},
	
	Editor={
		--Model="Editor/Objects/T.cgf",
		Icon="Tornado.bmp",
	},
}


function Tornado:OnInit()
	self:OnReset();
end

function Tornado:OnPropertyChange()
	self:OnReset();
end

function Tornado:OnReset()
end

function Tornado:OnShutDown()
end

function Tornado:Event_TargetReached( sender )
end

function Tornado:Event_Enable( sender )
end

function Tornado:Event_Disable( sender )
end

Tornado.FlowEvents =
{
	Inputs =
	{
		Disable = { Tornado.Event_Disable, "bool" },
		Enable = { Tornado.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		TargetReached = "bool",
	},
}
