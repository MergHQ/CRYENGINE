LightningArc =
{
	Properties =
	{
		bActive = 1,
		
		Timing = {
			fDelay = 2,
			fDelayVariation = 0.5,
			
		},
		
		Render = {
			ArcPreset = "Default",
		},
	},
	
	Editor =
	{
		Model="Editor/Objects/Particles.cgf",
		Icon="Lightning.bmp",
	},
}


function LightningArc:OnPropertyChange()
	self.lightningArc:ReadLuaParameters();
end



function LightningArc:Event_Strike()
	self.lightningArc:TriggerSpark();
end



function LightningArc:Event_Enable()
	self.lightningArc:Enable(true);
end



function LightningArc:Event_Disable()
	self.lightningArc:Enable(false);
end



function LightningArc:OnStrike(strikeTime, targetEntity)
	self:ActivateOutput("StrikeTime", strikeTime);
	self:ActivateOutput("EntityId", targetEntity.id);
end



LightningArc.FlowEvents =
{
	Inputs =
	{
		Strike = { LightningArc.Event_Strike, "bool" },
		Enable = { LightningArc.Event_Enable, "bool" },
		Disable = { LightningArc.Event_Disable, "bool" },
	},
	Outputs =
	{
		StrikeTime = "float",
		EntityId = "entity",
	},
}
