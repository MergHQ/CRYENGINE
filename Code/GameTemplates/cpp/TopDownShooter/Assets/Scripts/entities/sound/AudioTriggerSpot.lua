Script.ReloadScript("scripts/Entities/Sound/Shared/AudioUtils.lua");

AudioTriggerSpot = {
	Editor={
		Model="Editor/Objects/Sound.cgf",
		Icon="Sound.bmp",
	},
	
	Properties = {
		bEnabled = true,
		audioTriggerPlayTriggerName = "",
		audioTriggerStopTriggerName = "",
		bSerializePlayState = true, -- Determines if execution after de-serialization is needed.
		bTriggerAreasOnMove = false, -- Triggers area events or not. (i.e. dynamic environment updates on move)
		eiSoundObstructionType = 1, -- Clamped between 1 and 5. 1=ignore, 2=adaptive, 3=low, 4=medium, 5=high
			
		PlayMode = {
			eiBehaviour = 0, -- 0=Single, 1=Delay, 2=TriggerRate
			fMinDelay = 1,
			fMaxDelay = 2,
			vectorRandomizationArea = {x=0.0, y=0.0, z=0.0},
		},
		
		Debug = {
			eiDrawActivityRadius = 0,
			bDrawRandomizationArea = false,
		}
	},
	
	hOnTriggerID = nil,
	hOffTriggerID = nil,
	hCurrentOnTriggerID = nil,
	hCurrentOffTriggerID = nil, -- only used in OnPropertyChange()
	tObstructionType = {},
	currentBehaviour = 0,
	
	bIsHidden = false,
	bIsPlaying = false,
	bWasPlaying = false,
	bHasMoved = false,
	bOriginalEnabled = true,
}

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_LookupTriggerIDs()
	self.hOnTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerPlayTriggerName);	
	self.hOffTriggerID = AudioUtils.LookupTriggerID(self.Properties.audioTriggerStopTriggerName);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_LookupObstructionSwitchIDs()
	-- cache the obstruction switch and state IDs
	self.tObstructionType = AudioUtils.LookupObstructionSwitchAndStates();
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_SetObstruction()
	local nStateIdx = self.Properties.eiSoundObstructionType;
	
	if ((self.tObstructionType.hSwitchID ~= nil) and (self.tObstructionType.tStateIDs[nStateIdx] ~= nil)) then
		self:SetAudioSwitchState(self.tObstructionType.hSwitchID, self.tObstructionType.tStateIDs[nStateIdx], self:GetDefaultAuxAudioProxyID());
	end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_GenerateOffset()
	local offset = {x=0, y=0, z=0};
	offset.x = randomF(-self.Properties.PlayMode.vectorRandomizationArea.x/2.0, self.Properties.PlayMode.vectorRandomizationArea.x/2.0);
	offset.y = randomF(-self.Properties.PlayMode.vectorRandomizationArea.y/2.0, self.Properties.PlayMode.vectorRandomizationArea.y/2.0);
	offset.z = randomF(-self.Properties.PlayMode.vectorRandomizationArea.z/2.0, self.Properties.PlayMode.vectorRandomizationArea.z/2.0);
	return offset;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnSpawn()	
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	
	if (self.Properties.bTriggerAreasOnMove) then
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 0);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
	else
		self:SetFlags(ENTITY_FLAG_TRIGGER_AREAS, 2);
		self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 2);
	end
	
	if (System.IsEditor()) then
		self:Activate(1);
	end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnSave(save)	
	save.Properties = self.Properties;
	save.bWasPlaying = self.bIsPlaying;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnLoad(load)
	self.Properties = load.Properties;
	self.bWasPlaying = load.bWasPlaying;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnPostLoad()
	if (self.Properties.bSerializePlayState) then
		if (self.bIsPlaying and not self.bWasPlaying) then
			self:Stop();
		elseif (not self.bIsPlaying and self.bWasPlaying) then
			self:Play();
		end
	end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:_Init()	
	self.bIsPlaying = false;
	self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
	self:NetPresent(0);
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnPropertyChange()	
	if (self.Properties.eiSoundObstructionType < 1) then
		self.Properties.eiSoundObstructionType = 1;
	elseif (self.Properties.eiSoundObstructionType > 5) then
		self.Properties.eiSoundObstructionType = 5;
	end
	
	if(self.Properties.PlayMode.vectorRandomizationArea.x < 0.0) then
		self.Properties.PlayMode.vectorRandomizationArea.x = 0.0;
	end
	if(self.Properties.PlayMode.vectorRandomizationArea.y < 0.0) then
		self.Properties.PlayMode.vectorRandomizationArea.y = 0.0;
	end
	if(self.Properties.PlayMode.vectorRandomizationArea.z < 0.0) then
		self.Properties.PlayMode.vectorRandomizationArea.z = 0.0;
	end
	
	self:_LookupTriggerIDs();
	self:_SetObstruction();
	self:SetCurrentAudioEnvironments();
	self:SetAudioProxyOffset(g_Vectors.v000, self:GetDefaultAuxAudioProxyID());
	
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
		
	if (self.currentBehaviour ~= self.Properties.PlayMode.eiBehaviour) then
		self:Stop();
		self:KillTimer(0);
		
		if (self.Properties.PlayMode.eiBehaviour == 0) then
			self:Play();
		else
			self:SetTimer(0, 1000 * randomF(self.Properties.PlayMode.fMinDelay, self.Properties.PlayMode.fMaxDelay));
		end

		self.currentBehaviour = self.Properties.PlayMode.eiBehaviour;
	else
		if (not self.bIsPlaying) then
			-- Try to play, if disabled, hidden or invalid on-trigger Play() will fail!
			self:Play();
		end
	end
		
	if (not self.Properties.bEnabled and ((self.bOriginalEnabled) or (self.hCurrentOffTriggerID ~= self.hOffTriggerID))) then
		self.hCurrentOffTriggerID = self.hOffTriggerID;
		self:Stop(); -- stop if disabled, either stops running StartTrigger or executes StopTrigger!
	end
	
	self.bOriginalEnabled = self.Properties.bEnabled;
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnReset(bToGame)
	if (bToGame) then
		-- store the entity's "bEnabled" property's value so we can adjust back to it if changed over the course of the game
		self.bOriginalEnabled = self.Properties.bEnabled;
		
		-- re-execute this ATS once upon entering game mode
		self:Stop();
		self:Play();
	else
		self.Properties.bEnabled = self.bOriginalEnabled;
	end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:OnTransformFromEditorDone()
	self:SetCurrentAudioEnvironments();
end

----------------------------------------------------------------------------------------
AudioTriggerSpot["Server"] = {
	OnInit= function (self)
		self:_Init();
	end,
	OnShutDown= function (self)
	end,
}

----------------------------------------------------------------------------------------
AudioTriggerSpot["Client"] = {
	----------------------------------------------------------------------------------------
	OnInit = function(self)
		self:_Init();
		self:_LookupTriggerIDs();
		self:_LookupObstructionSwitchIDs();
		self:_SetObstruction();
		self:SetCurrentAudioEnvironments();
		self:Play();
	end,
	
	----------------------------------------------------------------------------------------
	OnShutDown = function(self)
		self:Stop();
	end,
	
	----------------------------------------------------------------------------------------
	OnSoundDone = function(self, hTriggerID)
		if (self.hOnTriggerID == hTriggerID) then
			self:ActivateOutput( "Done",true );
			if (self.Properties.PlayMode.eiBehaviour == 1) then
				self:SetTimer(0, 1000 * randomF(self.Properties.PlayMode.fMinDelay, self.Properties.PlayMode.fMaxDelay));
			end
		end
	end,
	
	----------------------------------------------------------------------------------------
    OnTimer = function(self, timerid, msec)    
        if (timerid == 0) then
            self:Play();
        end
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
	OnMove = function(self)
		self.bHasMoved = true;
	end,
	
	----------------------------------------------------------------------------------------
	OnUpdate = function(self)
		local pos = self:GetWorldPos();
		
		if (System.IsEditor() and (not self.bIsHidden)) then
		
			if (self.Properties.Debug.bDrawRandomizationArea) then
				local area = self.Properties.PlayMode.vectorRandomizationArea;
				local rot = self:GetWorldAngles();
				System.DrawOBB(pos.x, pos.y, pos.z, area.x, area.y, area.z, rot.x, rot.y, rot.z);
			end
			
			if(self.Properties.Debug.eiDrawActivityRadius > 0) then
				if ((self.hOnTriggerID ~= nil) and (self.Properties.Debug.eiDrawActivityRadius == 1))then
					local radius = Sound.GetAudioTriggerRadius(self.hOnTriggerID);
					System.DrawSphere(pos.x, pos.y, pos.z, radius, 250, 100, 100, 100);
						
					local fadeOutDistance = Sound.GetAudioTriggerOcclusionFadeOutDistance(self.hOnTriggerID);
					if(fadeOutDistance > 0.0) then
						System.DrawSphere(pos.x, pos.y, pos.z, radius - fadeOutDistance, 200, 200, 255, 100);
					end
				end
				if ((self.hOffTriggerID ~= nil) and (self.Properties.Debug.eiDrawActivityRadius == 2))then
					local radius = Sound.GetAudioTriggerRadius(self.hOffTriggerID);
					System.DrawSphere(pos.x, pos.y, pos.z, radius, 250, 100, 100, 100);
						
					local fadeOutDistance = Sound.GetAudioTriggerOcclusionFadeOutDistance(self.hOffTriggerID);
					if(fadeOutDistance > 0.0) then
						System.DrawSphere(pos.x, pos.y, pos.z, radius - fadeOutDistance, 200, 200, 255, 100);
					end
				end
			end
		end
	end,
}

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Play()
	if ((self.hOnTriggerID ~= nil) and (self.Properties.bEnabled) and (not self.bIsHidden)) then
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
		
		if (self.Properties.PlayMode.eiBehaviour == 2) then
			self:SetTimer(0, 1000 * randomF(self.Properties.PlayMode.fMinDelay, self.Properties.PlayMode.fMaxDelay));
		end
	end
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Stop()
	if (not self.bIsHidden) then
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

------------------------------------------------------------------------------------------------------
-- Event Handlers
------------------------------------------------------------------------------------------------------
function AudioTriggerSpot:Event_Enable( sender )	
	if (not self.Properties.bEnabled) then
		self.Properties.bEnabled = true;
		self:Play();
	end
	--BroadcastEvent( self,"Enable" );
end

----------------------------------------------------------------------------------------
function AudioTriggerSpot:Event_Disable( sender )	
	self:Stop();
	self.Properties.bEnabled = false;
  --BroadcastEvent( self,"Disable" );
end

----------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------
AudioTriggerSpot.FlowEvents =
{
	Inputs =
	{
		Enable = { AudioTriggerSpot.Event_Enable, "bool" },
		Disable = { AudioTriggerSpot.Event_Disable, "bool" },
	},
	
	Outputs =
	{
	  Done = "bool",
	},
}
