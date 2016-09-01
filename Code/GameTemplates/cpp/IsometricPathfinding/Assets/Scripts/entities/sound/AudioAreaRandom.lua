-- audio area ambience entity - to be attached to an area
-- used for convenient implementation of area based audio ambiences

AudioAreaRandom = {
	type = "AudioAreaRandom",
	
	Editor={
		Model="Editor/Objects/Sound.cgf",
		Icon="AudioAreaRandom.bmp",
	},
	
	Properties = {
		bEnabled = true,
		bTriggerAreasOnMove = false, -- Triggers area events or not. (i.e. dynamic environment updates on move)
		bMoveWithEntity = false,
		audioTriggerPlayTrigger = "",
		audioTriggerStopTrigger = "",
		audioRTPCRtpc = "",
		eiSoundObstructionType = 1, -- Clamped between 1 and 5. 1=ignore, 2=adaptive, 3=low, 4=medium, 5=high
		fRtpcDistance = 5.0,
		fRadiusRandom = 10.0,
		fMinDelay = 1,
		fMaxDelay = 2,
	},
  
	fFadeValue = 0.0,
	nState = 0, -- 0 = far, 1 = near, 2 = inside
	hOnTriggerID = nil,
	hOffTriggerID = nil,
	hCurrentOnTriggerID = nil,
	hCurrentOffTriggerID = nil, -- only used in OnPropertyChange()
	hRtpcID = nil,
	tObstructionType = {},
	bIsHidden = false,
	bIsPlaying = false,
	bHasMoved = false,
	bOriginalEnabled = true,
}

----------------------------------------------------------------------------------------
function AudioAreaRandom:_LookupControlIDs()
	self.hOnTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerPlayTrigger);	
	self.hOffTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerStopTrigger);
	self.hRtpcID = AudioUtils.LookupRtpcID(self.Properties.audioRTPCRtpc);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_LookupObstructionSwitchIDs()
	-- cache the obstruction switch and state IDs
	self.tObstructionType = AudioUtils.LookupObstructionSwitchAndStates();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_SetObstruction()
	local nStateIdx = self.Properties.eiSoundObstructionType;
	
	if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
		self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
	end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_UpdateParameters()	
	self:SetFadeDistance(self.Properties.fRtpcDistance);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_UpdateRtpc()
	if (self.hRtpcID ~= nil) then
		self:SetAudioRtpcValue(self.hRtpcID, self.fFadeValue, self:GetDefaultAuxAudioProxyID());
	end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:_GenerateOffset()
	local offset = {x = 0, y = 0, z = 0}	
	offset.x = randomF(-1, 1);
	offset.y = randomF(-1, 1);
	NormalizeVector(offset);
	ScaleVectorInPlace(offset, randomF(0, self.Properties.fRadiusRandom));
	
	return offset;
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnSpawn()
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
function AudioAreaRandom:OnLoad(load)
	self.Properties = load.Properties;
	self.fFadeValue = load.fFadeValue;
	self.nState = 0; -- We start out being far, in a subsequent update we will determine our actual state!
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnPostLoad()
	self:_UpdateParameters();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnSave(save)
	save.Properties = self.Properties;
	save.fFadeValue = self.fFadeValue;
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnPropertyChange()
	if (self.Properties.eiSoundObstructionType < 1) then
		self.Properties.eiSoundObstructionType = 1;
	elseif (self.Properties.eiSoundObstructionType > 5) then
		self.Properties.eiSoundObstructionType = 5;
	end
	
	self:_LookupControlIDs();
	self:_UpdateParameters();
	self:_SetObstruction();
	self:SetCurrentAudioEnvironments();
	self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
	self:AuxAudioProxiesMoveWithEntity(self.Properties.bMoveWithEntity);
	
	if (self.Properties.bTriggerAreasOnMove) then
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 0);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
	else
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 2);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 2);
	end
	
	if ((self.bIsPlaying) and (self.hCurrentOnTriggerID ~= self.hOnTriggerID)) then
		-- Stop a possibly playing instance if the on-trigger changed!
		self:StopAudioTrigger(self.hCurrentOnTriggerID, self:GetDefaultAuxAudioProxyID());
		self.hCurrentOnTriggerID = self.hOnTriggerID;
		self.bIsPlaying = false;
		self.bHasMoved = false;
		self:KillTimer(0);
	end
	
	if (not self.bIsPlaying) then
		-- Try to play, if disabled, hidden or invalid on-trigger Play() will fail!
		self:Play();
	end
		
	if (not self.Properties.bEnabled and ((self.bOriginalEnabled) or (self.hCurrentOffTriggerID ~= self.hOffTriggerID))) then
		self.hCurrentOffTriggerID = self.hOffTriggerID;
		self:Stop(); -- stop if disabled, either stops running StartTrigger or executes StopTrigger!
	end
	
	self.bOriginalEnabled = self.Properties.bEnabled;
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:OnReset(bToGame)
	if (bToGame) then
		-- store the entity's "bEnabled" property's value so we can adjust back to it if changed over the course of the game
		self.bOriginalEnabled = self.Properties.bEnabled;
		
		-- re-execute this AAR once upon entering game mode
		self:Stop();
		self:Play();
	else
		self.Properties.bEnabled = self.bOriginalEnabled;
	end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:Play()
	if ((self.hOnTriggerID ~= nil) and (self.Properties.bEnabled) and (not self.bIsHidden) and ((self.nState == 1) or ((self.nState == 2)))) then		
		local offset = self:_GenerateOffset();		
		if (LengthSqVector(offset) > 0.00001) then-- offset is longer than 1cm
			self:SetAudioProxyOffset(offset, self:GetDefaultAuxAudioProxyID());
			self:SetCurrentAudioEnvironments();
		elseif (self.bHasMoved) then
			self:SetCurrentAudioEnvironments();
		end
		
		self:ExecuteAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
		self.bIsPlaying = true;
		self.bHasMoved = false;
		self.hCurrentOnTriggerID = self.hOnTriggerID;
		
		self:SetTimer(0, 1000 * randomF(self.Properties.fMinDelay, self.Properties.fMaxDelay));
	end
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:Stop()
	if (not self.bIsHidden and ((self.nState == 1) or ((self.nState == 2)))) then
		-- Cannot check against "self.bIsPlaying" otherwise we won't execute the StopTrigger if there's no StartTrigger set!
		if (self.hOffTriggerID ~= nil) then
			local offset = self:_GenerateOffset();
			if (LengthSqVector(offset) > 0.00001) then-- offset is longer than 1cm
				self:SetAudioProxyOffset(offset, self:GetDefaultAuxAudioProxyID());
				self:SetCurrentAudioEnvironments();
			elseif (self.bHasMoved) then
				self:SetCurrentAudioEnvironments();
			end
		
			self:ExecuteAudioTrigger(self.hOffTriggerID, self:GetDefaultAuxAudioProxyID());
		elseif (self.hOnTriggerID ~= nil) then
			self:StopAudioTrigger(self.hOnTriggerID, self:GetDefaultAuxAudioProxyID());
		end
	end
		
	self.bIsPlaying = false;
	self.bHasMoved = false;
	self:KillTimer(0);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:CliSrv_OnInit()
	self.nState = 0;
	self.fFadeValue = 0.0;
	self:SetFlags(ENTITY_FLAG_VOLUME_SOUND, 0);
	self:_UpdateParameters();
	self.bIsPlaying = false;
	self:NetPresent(0);
	self:AuxAudioProxiesMoveWithEntity(self.Properties.bMoveWithEntity);
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:UpdateFadeValue(player, distance)
	if (not(self.Properties.bEnabled)) then
		self.fFadeValue = 0.0;
		self:_UpdateRtpc();
		do return end;
	end
	
	if (self.Properties.fRtpcDistance > 0.0) then
		local fade = (self.Properties.fRtpcDistance - distance) / self.Properties.fRtpcDistance;
		fade = (fade > 0.0) and fade or 0.0;
		
		if (math.abs(self.fFadeValue - fade) > AudioUtils.areaFadeEpsilon) then
			self.fFadeValue = fade;
			self:_UpdateRtpc();
		end
	end
end

----------------------------------------------------------------------------------------
AudioAreaRandom.Server={
	OnInit = function(self)
		self:CliSrv_OnInit();
	end,
  
	OnShutDown = function(self)
	end,
}

----------------------------------------------------------------------------------------
AudioAreaRandom.Client={
	----------------------------------------------------------------------------------------
	OnInit = function(self)
		self:RegisterForAreaEvents(1);
		self:_LookupControlIDs();
		self:_LookupObstructionSwitchIDs();
		self:_SetObstruction();
		self:CliSrv_OnInit();
	end,
	
	----------------------------------------------------------------------------------------
	OnShutDown = function(self)
		self:Stop();
		self.nState = 0;
		self:RegisterForAreaEvents(0);
	end,
	
	----------------------------------------------------------------------------------------
	OnHidden = function(self)
		self:Stop();
		self.bIsHidden = true;
	end,
 
	----------------------------------------------------------------------------------------
	OnUnHidden = function(self)
		self.bIsHidden = false;
		self:Play();
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerEnterNearArea = function(self, player, areaId, distance)		
		if (self.nState == 0) then
			self.nState = 1;
			
			if (distance < self.Properties.fRtpcDistance) then
				self:Play();
				self.fFadeValue = 0.0;
				self:_UpdateRtpc();
			end
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerMoveNearArea = function(self, player, areaId, distance)
		self.nState = 1;
		
		if ((not self.bIsPlaying) and distance < self.Properties.fRtpcDistance) then
			self:Play();
			self:UpdateFadeValue(player, distance);
		elseif ((self.bIsPlaying) and distance > self.Properties.fRtpcDistance) then
			self:Stop();
			self:UpdateFadeValue(player, distance);
		elseif (distance < self.Properties.fRtpcDistance) then
			self:UpdateFadeValue(player, distance);
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerEnterArea = function(self, player)
		if (self.nState == 0) then
			-- possible if the listener is teleported or gets spawned inside the area
			-- technically, the listener enters the Near Area and the Inside Area at the same time
			self.nState = 2;
			self:Play();
		else
			self.nState = 2;
		end
		
		self.fFadeValue = 1.0;
		self:_UpdateRtpc();
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerProceedFadeArea = function(self, player, fade)
		-- normalized fade value depending on the "InnerFadeDistance" set to an inner, higher priority area
		self.nState = 2;
		
		if ((math.abs(self.fFadeValue - fade) > AudioUtils.areaFadeEpsilon) or ((fade == 0.0) and (self.fFadeValue ~= fade))) then
			self.fFadeValue = fade;
			self:_UpdateRtpc();
			
			if ((not self.bIsPlaying) and (fade > 0.0)) then
				self:Play();
			elseif ((self.bIsPlaying) and (fade == 0.0)) then
				self:Stop();
			end
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnAudioListenerLeaveArea = function(self, player, areaId, fade)
		self.nState = 1;
	end,
	
	----------------------------------------------------------------------------------------	
	OnAudioListenerLeaveNearArea = function(self, player, areaId, fade)
		if (self.bIsPlaying) then
			self:Stop();
		end
		
		self.nState = 0;
		self.fFadeValue = 0.0;
		self:_UpdateRtpc();
	end,
	
	----------------------------------------------------------------------------------------
	OnBindThis = function(self)
		self:RegisterForAreaEvents(1);
	end,
	
	----------------------------------------------------------------------------------------
	OnUnBindThis = function(self)
		self:Stop();
		self.nState = 0;
		self:RegisterForAreaEvents(0);
	end,
	
	----------------------------------------------------------------------------------------
	OnTimer = function(self, timerid, msec)
		if (timerid == 0) then
			self:Play();
		end
	end,
	
	----------------------------------------------------------------------------------------
	OnMove = function(self)
		self.bHasMoved = true;
	end,
}

----------------------------------------------------------------------------------------
-- Event Handlers
----------------------------------------------------------------------------------------
function AudioAreaRandom:Event_Enable(sender)
	self.Properties.bEnabled = true;
	self:OnPropertyChange();
end

----------------------------------------------------------------------------------------
function AudioAreaRandom:Event_Disable(sender)
	self.Properties.bEnabled = false;
	self:OnPropertyChange();
end

----------------------------------------------------------------------------------------
AudioAreaRandom.FlowEvents =
{
	Inputs =
	{
		Enable = { AudioAreaRandom.Event_Enable, "bool" },
		Disable = { AudioAreaRandom.Event_Disable, "bool" },
	},
}
