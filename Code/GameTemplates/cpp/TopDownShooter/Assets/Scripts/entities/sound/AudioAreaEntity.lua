-- audio area entity - to be attached to an area
-- reports a normalized (0-1) fade value depending on the listener's distance to the area

AudioAreaEntity = {
	type = "AudioAreaEntity",
	
	Editor={
		Model="Editor/Objects/Sound.cgf",
		Icon="AudioAreaEntity.bmp",
	},
	
	Properties = {
		bEnabled = true,
		bTriggerAreasOnMove = false, -- Triggers area events or not. (i.e. dynamic environment updates on move)
		audioEnvironmentEnvironment = "",
		eiSoundObstructionType = 1, -- Clamped between 1 and 5. 1=ignore, 2=adaptive, 3=low, 4=medium, 5=high
		fFadeDistance = 5.0,
		fEnvironmentDistance = 5.0,
	},
	
	fFadeValue = 0.0,
	nState = 0, -- 0 = far, 1 = near, 2 = inside
	fFadeOnUnregister = 1.0,
	hEnvironmentID = nil,
	tObstructionType = {},
	bIsActive = false,
}

----------------------------------------------------------------------------------------
function AudioAreaEntity:_ActivateOutput(sPortName, value)
	if (self.Properties.bEnabled) then
		self:ActivateOutput(sPortName, value);
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:_UpdateParameters()
	-- Set the distances as the very first thing!
	self:SetFadeDistance(self.Properties.fFadeDistance);
	self:SetEnvironmentFadeDistance(self.Properties.fEnvironmentDistance);
	
	if (self.Properties.bEnabled) then
		self.hEnvironmentID = AudioUtils.LookupAudioEnvironmentID(self.Properties.audioEnvironmentEnvironment);
		if (self.hEnvironmentID ~= nil) then
			self:SetAudioEnvironmentID(self.hEnvironmentID);
		end
	else
		self:SetAudioEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID);
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:_LookupObstructionSwitchIDs()
	-- cache the obstruction switch and state IDs
	self.tObstructionType = AudioUtils.LookupObstructionSwitchAndStates();
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:_SetObstruction()
	local nStateIdx = self.Properties.eiSoundObstructionType;
	
	if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
		self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:_DisableObstruction()
	-- Ignore is at index 1
	local nStateIdx = 1;
	
	if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
		self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	
	if (self.Properties.bTriggerAreasOnMove) then
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 0);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
	else
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 2);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 2);
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:OnLoad(load)
	self.Properties	= load.Properties;
	self.fFadeOnUnregister = load.fFadeOnUnregister;
	self.nState = 0; 
	self.fFadeValue = 0.0; 
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:OnPostLoad()
	self:_UpdateParameters();
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:OnSave(save)
	save.Properties = self.Properties;
	save.fFadeOnUnregister = self.fFadeOnUnregister;
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:OnPropertyChange()
	if (self.Properties.eiSoundObstructionType < 1) then
		self.Properties.eiSoundObstructionType = 1;
	elseif (self.Properties.eiSoundObstructionType > 5) then
		self.Properties.eiSoundObstructionType = 5;
	end
	
	self:_UpdateParameters();
	
	if (self.Properties.bTriggerAreasOnMove) then
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 0);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
	else
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 2);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 2);
	end
	
	if (self.nState == 1) then -- near
		self:_SetObstruction();
	elseif (self.nState == 2) then -- inside
		self:_DisableObstruction();
	end
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:CliSrv_OnInit()
	self.nState = 0;
	self.fFadeValue = 0.0;
	self.fFadeOnUnregister = 1.0;
	self:SetFlags(ENTITY_FLAG_VOLUME_SOUND, 0);
 
	self:_UpdateParameters();
end

----------------------------------------------------------------------------------------
function AudioAreaEntity:UpdateFadeValue(player, distance)
	if (not(self.Properties.bEnabled)) then
		self.fFadeValue = 0.0;
		self:_ActivateOutput("FadeValue", self.fFadeValue);
		do return end;
	end
	
	if (self.Properties.fFadeDistance > 0.0) then
		local fade = (self.Properties.fFadeDistance - distance) / self.Properties.fFadeDistance;
		fade = (fade > 0.0) and fade or 0.0;
		
		if (math.abs(self.fFadeValue - fade) > AudioUtils.areaFadeEpsilon) then
			self.fFadeValue = fade;
			self:_ActivateOutput("FadeValue", self.fFadeValue);
		end
	end
end

----------------------------------------------------------------------------------------
AudioAreaEntity.Server={
	OnInit = function(self)
		self:CliSrv_OnInit();
	end,
	
	OnShutDown = function(self)
	end,
}

----------------------------------------------------------------------------------------
AudioAreaEntity.Client={
	----------------------------------------------------------------------------------------
	OnInit = function(self)
		self:RegisterForAreaEvents(1);
		self:_LookupObstructionSwitchIDs();
		self:_SetObstruction();
		self:CliSrv_OnInit();
	end,
	
	----------------------------------------------------------------------------------------
	OnShutDown = function(self)
		self.nState = 0;
		self:RegisterForAreaEvents(0);
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerEnterNearArea = function(self, player, areaId, distance)
		if (self.nState == 0) then
			if (distance < self.Properties.fFadeDistance) then
				self.bIsActive = true;
				self.fFadeValue = 0.0;
				self:_ActivateOutput("OnFarToNear", true);
				self:_ActivateOutput("FadeValue", self.fFadeValue);
			end
		elseif (self.nState == 2) then
			self.fFadeValue = fade;
			self:_ActivateOutput("OnInsideToNear", true);
			self:_ActivateOutput("FadeValue", self.fFadeValue);
		end
		
		self.nState = 1;
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerMoveNearArea = function(self, player, areaId, distance)
		self.nState = 1;
		
		if ((not self.bIsActive) and distance < self.Properties.fFadeDistance) then
			self.bIsActive = true;
			self:_ActivateOutput("OnFarToNear", true);
			self:UpdateFadeValue(player, distance);
		elseif ((self.bIsActive) and distance > self.Properties.fFadeDistance) then
			self:_ActivateOutput("OnNearToFar", true);
			self:UpdateFadeValue(player, distance);
			self.bIsActive = false;
		elseif (distance < self.Properties.fFadeDistance) then
			self:UpdateFadeValue(player, distance);
		end
	end,	
	
	----------------------------------------------------------------------------------------
	OnAudioListenerEnterArea = function(self, player)
		if (self.nState == 0) then
			-- possible if the player is teleported or gets spawned inside the area
			-- technically, the listener enters the Near Area and the Inside Area at the same time
			self.bIsActive = true;
			self:_ActivateOutput("OnFarToNear", true);
		end
		
		self.nState = 2;
		self.fFadeValue = 1.0;
		self:_ActivateOutput("OnNearToInside", true);
		self:_ActivateOutput("FadeValue", self.fFadeValue);
		self:_DisableObstruction();
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerProceedFadeArea = function(self, player, fade)
		-- normalized fade value depending on the "InnerFadeDistance" set to an inner, higher priority area
		self.nState = 2;
		
		if ((math.abs(self.fFadeValue - fade) > AudioUtils.areaFadeEpsilon) or ((fade == 0.0) and (self.fFadeValue ~= fade))) then
			self.fFadeValue = fade;
			self:_ActivateOutput("FadeValue", self.fFadeValue);
			
			if ((not self.bIsActive) and (fade > 0.0)) then
				self.bIsActive = true;
				self:_ActivateOutput("OnFarToNear", true);
				self:_ActivateOutput("OnNearToInside", true);
			elseif ((self.bIsActive) and (fade == 0.0)) then
				self:_ActivateOutput("OnInsideToNear", true);
				self:_ActivateOutput("OnNearToFar", true);
				self.bIsActive = false;
			end
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerLeaveArea = function(self, player, areaId, fade)
		self.nState = 1;
		self:_ActivateOutput("OnInsideToNear", true);
		self:_SetObstruction();
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerLeaveNearArea = function(self, player, areaId, fade)
		self.nState = 0;
		self.fFadeValue = 0.0;
		
		if (self.bIsActive) then
			self:_ActivateOutput("OnNearToFar", true);
			self:_ActivateOutput("FadeValue", self.fFadeValue);
			self.bIsActive = false;
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnBindThis = function(self)
		self:RegisterForAreaEvents(1);
	end,
	
	----------------------------------------------------------------------------------------
	OnUnBindThis = function(self)
		self.nState = 0;
		self:RegisterForAreaEvents(0);
	end,
}

----------------------------------------------------------------------------------------
-- Event Handlers
----------------------------------------------------------------------------------------
function AudioAreaEntity:Event_Enable(sender)
	self.Properties.bEnabled = true;
	self:OnPropertyChange();
end

function AudioAreaEntity:Event_Disable(sender)
	self.Properties.bEnabled = false;
	self:OnPropertyChange();
end

AudioAreaEntity.FlowEvents =
{
	Inputs =
	{
		Enable = { AudioAreaEntity.Event_Enable, "bool" },
		Disable = { AudioAreaEntity.Event_Disable, "bool" },
	},
	
	Outputs =
	{
		FadeValue = "float",
		OnFarToNear = "bool",
		OnNearToInside = "bool",
		OnInsideToNear = "bool",
		OnNearToFar = "bool",
	},
}
