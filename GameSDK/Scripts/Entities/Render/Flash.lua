Flash =
{
	Properties =
	{
		Timing = {
			fFadeInTime = 0.1,  -- Fade in time
			fFadeOutTime = 0.1,  -- Fade in time
			fFlashDuration = 1,  -- in seconds
		},
		Effects = {
			SkyHighlightMultiplier = 1,
			color_SkyHighlightColor = {x=0.8,y=0.8,z=1},
			SkyHighlightAtten = 10,
			sound_Sound = "",
		},
	},

	TempPos = {x=0.0,y=0.0,z=0.0},

	Editor={
		Icon="Flash.bmp",
	},

	_SkyHighlight = { size=0,color={x=0,y=0,z=0},position={x=0,y=0,z=0} },
}

function Flash:OnInit()
	self.bStriking = 0;
	self.light_fade = 0;
	self.time = 0;
	self.state = 0;
	
	self.light_intensity = 0;
	self.vStrikeOffset = {x=0,y=0,z=0};
	self.vSkyHighlightPos = {x=0,y=0,z=0};
end

function Flash:OnShutDown()
	self:StopStrike();
end

function Flash:StopStrike()
	self.bStriking = 0;
	self.bStopStrike = 0;
	self.state = 0;
	
	-- Restore zero sky highlight.
	self._SkyHighlight.size = 0;
	self._SkyHighlight.color.x = 0;
	self._SkyHighlight.color.y = 0;
	self._SkyHighlight.color.z = 0;
	self._SkyHighlight.position.x = 0;
	self._SkyHighlight.position.y = 0;
	self._SkyHighlight.position.z = 0;
	System.SetSkyHighlight( self._SkyHighlight );
	
	self:Activate(0);
end

function Flash:OnLoad(table)
	self:StopStrike();
	
	self.bStriking = table.bStriking;
	self.light_fade = table.light_fade;
	self.time = table.time;
	self.state = table.state;
	
	self.light_intensity = table.light_intensity;
	self.vStrikeOffset = table.vStrikeOffset;
	self.vSkyHighlightPos = table.vSkyHighlightPos;
	
	if (table.bActive == true) then
		self:Activate(1);
	end
end

function Flash:OnSave(table)
	table.bActive = self:IsActive();
	
	table.bStriking = self.bStriking;
	table.light_fade = self.light_fade;
	table.time = self.time;
	table.state = self.state;
	table.light_intensity = self.light_intensity;
	table.vStrikeOffset = self.vStrikeOffset;
	table.vSkyHighlightPos = self.vSkyHighlightPos;
end

function Flash:OnUpdate( dt )
	if (self.state == 0) then
		-- Fade in
		local delay = self.Properties.Timing.fFadeInTime;
		self.light_fade = 1 / delay;
		self.time = self.time + dt;
		
		if (self.time > delay) then
			self.state = 1;
			self.time = 0;
		end
	elseif (self.state == 1) then
		local delay = self.Properties.Timing.fFlashDuration;
		self.light_intensity = 1;
		self.light_fade = 0;
		self.time = self.time + dt;
		if (self.time > delay) then
			self.state = 2;
			self.time = 0;
		end
	elseif (self.state == 2) then
		local delay = self.Properties.Timing.fFadeOutTime;
		self.light_fade =  - 1 / delay;
		self.time = self.time + dt;
		if (self.time > delay) then
			self.time = 0;
			self:StopStrike();
		end
	end 
	
	self.light_intensity = self.light_intensity + self.light_fade*dt;

	if (self.light_intensity > 1) then
		self.light_intensity = 1;
	end
	if (self.light_intensity < 0) then
		self.light_intensity = 0;
	end
	
	self:UpdateFlashParams();
end

function Flash:UpdateFlashParams()
	local Effects = self.Properties.Effects;
	
	local highlight = self.light_intensity * Effects.SkyHighlightMultiplier;
	self._SkyHighlight.color.x = highlight * Effects.color_SkyHighlightColor.x;
	self._SkyHighlight.color.y = highlight * Effects.color_SkyHighlightColor.y;
	self._SkyHighlight.color.z = highlight * Effects.color_SkyHighlightColor.z;
	
	self._SkyHighlight.size = Effects.SkyHighlightAtten;
	
	self._SkyHighlight.position.x = self.vSkyHighlightPos.x;
	self._SkyHighlight.position.y = self.vSkyHighlightPos.y;
	self._SkyHighlight.position.z = self.vSkyHighlightPos.z;
	
	System.SetSkyHighlight( self._SkyHighlight );
end

------------------------------------------------------------------------------------------------------
-- Start lightning effect stopped in OnTimer
------------------------------------------------------------------------------------------------------
function Flash:Event_Strike()
	if (self.bStriking == 0) then
		self.bStriking = 1;
		
		local props = self.Properties;
		local Effects = props.Effects;
		
		local vEntityPos = self:GetPos();
		
		self.vSkyHighlightPos.x = vEntityPos.x;
		self.vSkyHighlightPos.y = vEntityPos.y;
		self.vSkyHighlightPos.z = vEntityPos.z;
		
		self.bStriking = 0;
		self.light_fade = 0;
		self.time = 0;
		self.state = 0;
		self.light_intensity = 0;
		
		self.light_fade = 1 / self.Properties.Timing.fFadeInTime;
			
		self:UpdateFlashParams();
		
		-- Play Flash sound.
		if (Effects.sound_Sound ~= "") then
		-- REINSTANTIATE!!!
		--	local sndFlags = bor(SOUND_DEFAULT_2D,0);
		--	self.soundid = self:PlaySoundEvent(Effects.sound_Sound, g_Vectors.v000, g_Vectors.v010, sndFlags, 0, SOUND_SEMANTIC_AMBIENCE_ONESHOT);
		end

		self:Activate(1);
	end
	BroadcastEvent( self,"Strike" );
end

------------------------------------------------------------------------------------------------------
-- Start lightning effect stopped in OnTimer
------------------------------------------------------------------------------------------------------
function Flash:Event_Stop()
	self:StopStrike();
	BroadcastEvent( self,"Stop" );
end

------------------------------------------------------------------------------------------------------
-- Event descriptions.
------------------------------------------------------------------------------------------------------
Flash.FlowEvents =
{
	Inputs =
	{
		Strike = { Flash.Event_Strike,"bool" },
		Stop = { Flash.Event_Stop,"bool" },
	},
	Outputs =
	{
		Strike = "bool",
		Stop = "bool",
	},
}
