--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------

AnimDoor =
{
	Server = {},
	Client = {},

	Properties =
	{
		soclasses_SmartObjectClass = "Door",
		object_Model = "",
		Sounds =
		{
			snd_Open = "",
			snd_Close = "",
		},
		Animation =
		{
			anim_Open="Default",
			anim_Close="",
		},
		Physics = 
		{
			bPhysicalize = 1, -- True if object should be physicalized at all.
			bRigidBody = 1, -- True if rigid body, False if static.
			bPushableByPlayers = 1,
			Density = -1,
			Mass = -1,
		},
		fUseDistance = 2.5,
		bLocked = 0,
		bActivatePortal = 0,
		bNoFriendlyFire = 0,
	},

	Editor =
	{
		Icon="Door.bmp",
		ShowBounds = 1,
	},

	nDirection = -1, -- -1 closed, 0 nothing, 1=open
	bUseSameAnim = 0,
	bNoAnims = 0,
	nSoundId = 0,
	bLocked = false,
	bNeedUpdate = 0
}

-------------------------------------------------------
function AnimDoor:OnLoad(table)
	self.bLocked = table.bLocked;
	self.bNeedUpdate = 0;
	-- start from default pose
	self:ResetAnimation(0, -1);
	self:DoStopSound();
	self.curAnim = "";
	self.nDirection = 0;
	if AI then
		AI.SetSmartObjectState( self.id, "Closed" );
	end
	if (self.bLocked and AI) then
		self:Lock();
	end

	-- deactivate portal by default
	if (self.portal) then
		System.ActivatePortal(self:GetWorldPos(), 0, self.id);
	end

	local newDirection = table.nDirection;

	if (table.doPlay == 1) then
		self:DoPlayAnimation(newDirection, table.animTime);
		-- DoPlayAnimation will also activate portal
	else
		-- need to set set according to newDirection
		if (newDirection == 1) then
			-- only need to set open state
			-- play last frame of opening animation
			local wantedTime = self:GetAnimationLength(0, self.Properties.Animation.anim_Open);
			self:DoPlayAnimation(newDirection, wantedTime, false); -- will also activate portal
			self.curAnim = "";
			if AI then
				AI.ModifySmartObjectStates( self.id, "Open-Closed" );
			end
		end
	end
end

-------------------------------------------------------
function AnimDoor:OnSave(table)
	table.bLocked = self.bLocked;

	if (self.curAnim ~= "" and self.nDirection ~= 0 and self:IsAnimationRunning(0,0)) then
		table.doPlay = 1;
		table.nDirection = self.nDirection;
		table.animTime = self:GetAnimationTime(0,0);
	else
		-- no need to play, but need to re-set set on load
		table.doPlay = 0;
		table.nDirection = self.nDirection;
	end
end

------------------------------------------------------------------------------------------------------
-- OnPropertyChange called only by the editor.
------------------------------------------------------------------------------------------------------
function AnimDoor:OnPropertyChange()
	self:Reset();
end

------------------------------------------------------------------------------------------------------
-- OnReset called only by the editor.
------------------------------------------------------------------------------------------------------
function AnimDoor:OnReset()
	self:Reset();
end

------------------------------------------------------------------------------------------------------
-- OnSpawn called Editor/Game.
------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------
function AnimDoor:OnSpawn()
	self:Reset();
end

function AnimDoor:Reset()
	if (self.portal) then
		System.ActivatePortal(self:GetWorldPos(), 0, self.id);
	end

	self.bLocked = false;
	self.portal = self.Properties.bActivatePortal~=0;
	self.bUseSameAnim = self.Properties.Animation.anim_Close == "";
	if (self.Properties.object_Model ~= "") then
		self:LoadObject(0,self.Properties.object_Model);
	end

	self.bNoAnims = self.Properties.Animation.anim_Open == "" and self.Properties.Animation.anim_Close == "";

	self:PhysicalizeThis();
	self:DoStopSound();

	-- state setting, closed
	self.nDirection = -1;
	self.curAnim = "";
	if AI then
		AI.SetSmartObjectState( self.id, "Closed" );
	end
	if (self.Properties.bLocked ~= 0) then
		self:Lock();
	end
end

function AnimDoor:PhysicalizeThis()
	local Physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid( self, 0, Physics, 1 );
end

function AnimDoor:IsUsable(user)
	if (not user) then
		return 0;
	end

	if (self.bLocked and self.nDirection == -1) then
	  return 0;
	end

 	local useDistance = self.Properties.fUseDistance;
	if (useDistance <= 0.0) then
		return 0;
	end

	local delta = g_Vectors.temp_v1;
	local mypos,bbmax = self:GetWorldBBox();

	FastSumVectors(mypos,mypos,bbmax);
	ScaleVectorInPlace(mypos,0.5);
	user:GetWorldPos(delta);

	SubVectors(delta,delta,mypos);

	local useDistance = self.Properties.fUseDistance;
	if (LengthSqVector(delta)<useDistance*useDistance) then
		return 1;
	else
		return 0;
	end
end

function AnimDoor:GetUsableMessage(idx)
	return "@use_door"
end

function AnimDoor:OnUsed(user)
	if (self.nDirection == 0) then
		return;
	end

	local newDir = -self.nDirection;
	if (newDir == 1) then
		self:Event_Open();
	else
		self:Event_Close();
	end
end


function AnimDoor:Lock()
	if AI then
		AI.ModifySmartObjectStates( self.id, "Locked" );
	end
	self.bLocked=true;
end;

function AnimDoor:Unlock()
	if AI then
		AI.ModifySmartObjectStates( self.id, "-Locked" );
	end
	self.bLocked=false;
end;

function AnimDoor.Server:OnUpdate(dt)
	if (self.bNeedUpdate == 0) then
	  return
	end

	if (self.bNoAnims ~= 0 or (self.curAnim ~= "" and self.nDirection ~= 0)) then
		if (not self:IsAnimationRunning(0,0)) then
			self.curAnim = "";
			if (self.nDirection == -1) then
				-- fully closed

				--System.Log("Closed");
				if AI then
					AI.ModifySmartObjectStates( self.id, "Closed-Open" );
				end
				-- deactivate portal when fully closed
				if (self.portal) then
					System.ActivatePortal(self:GetWorldPos(), 0, self.id);
				end
				self:Activate(0);
				self.bNeedUpdate = 0;
				BroadcastEvent(self, "Close");
			else
				-- fully open
				--System.Log("Opened");
				if AI then
					AI.ModifySmartObjectStates( self.id, "Open-Closed" );
				end
				self:Activate(0);
				self.bNeedUpdate = 0;
				BroadcastEvent(self, "Open");
			end
	  end
	end
end

function AnimDoor:DoPlaySound(sndName)
--REINST
	-- self:DoStopSound();
	-- if (sndName and sndName ~= "") then
		-- local sndFlags=bor(SOUND_DEFAULT_3D, 0);
		-- g_Vectors.temp = self:GetDirectionVector(1);
		-- self.nSoundId=self:PlaySoundEvent(sndName, g_Vectors.v000, g_Vectors.temp, sndFlags, 0, SOUND_SEMANTIC_MECHANIC_ENTITY);
	-- end
end;

function AnimDoor:DoStopSound()
	--REINST
	-- if(self.nSoundId ~= 0 and Sound.IsPlaying(self.nSoundId)) then
		-- self:StopSound(self.nSoundId);
	-- end
	-- self.nSoundId = 0;
end

function AnimDoor:DoPlayAnimation(direction, forceTime, useSound)
	if (self.nDirection == direction) then
	  return
	end

	-- stop animation
	local curTime = 0;

	local len = 0;
	local bNeedAnimStart = 1;
	if (self.curAnim~="" and self:IsAnimationRunning(0,0)) then
		curTime = self:GetAnimationTime(0,0);
		len = self:GetAnimationLength(0, self.curAnim);
		bNeedAnimStart = not self.bUseSameAnim;
	end

	if (bNeedAnimStart) then
		local animDirection = direction;
		local animName = self.Properties.Animation.anim_Open;
		if (direction == -1 and not self.bUseSameAnim) then
			animName = self.Properties.Animation.anim_Close;
			animDirection = -animDirection;
		end

		if (not self.bNoAnims) then
			self:StopAnimation(0,0);
			self:StartAnimation(0, animName);
			self:SetAnimationSpeed( 0, 0, animDirection );

			if (forceTime) then
				self:SetAnimationTime(0,0,forceTime);
			else
				-- now we have to match the times
				local percentage = 0.0;
				if (len > 0.0) then
					percentage = 1.0 - curTime / len;
					if (percentage > 1.0) then
					  percentage = 1.0;
					end
				end
				if (animDirection == -1) then
					percentage = 1.0 - percentage;
				end
				curTime = self:GetAnimationLength(0, animName) * percentage;
				self:SetAnimationTime(0,0,curTime);
			end
		end
		self.curAnim = animName;
	else
	  self:SetAnimationSpeed(0,0, direction);
	end

	self.nDirection = direction;
	self:ForceCharacterUpdate(0, true);
	self:Activate(1);
	self.bNeedUpdate = 1;

	-- activate portal on opening and on closing!
	if (self.portal) then
		System.ActivatePortal(self:GetWorldPos(), 1, self.id);
	end

	-- play sound after enabling the portal so sound doesn't get culled
	local sndName = self.Properties.Sounds.snd_Open;
	if (direction == -1) then
	  sndName = self.Properties.Sounds.snd_Close;
	end

	if (useSound == nil or useSound) then
		self:DoPlaySound(sndName);
	end

end

----------------------------------------------------------------------------------
------------------------------------Events----------------------------------------
----------------------------------------------------------------------------------
function AnimDoor:Event_Unlock()
	self:Unlock();
	BroadcastEvent(self, "Unlock");
end;

function AnimDoor:Event_Lock()
	self:Lock();
	BroadcastEvent(self, "Lock");
end;

function AnimDoor:Event_Open()
	--self:Open();
	self:DoPlayAnimation(1);
end;

function AnimDoor:Event_Close()
	--self:Close();
	self:DoPlayAnimation(-1);
end;

------------------------------------------------------------------------------------------------------
function AnimDoor:Event_Hide()
	self:Hide(1);
	self:ActivateOutput( "Hide", true );
end

------------------------------------------------------------------------------------------------------
function AnimDoor:Event_UnHide()
	self:Hide(0);
	self:ActivateOutput( "UnHide", true );
end

AnimDoor.FlowEvents =
{
	Inputs =
	{
		Close = { AnimDoor.Event_Close, "bool" },
		Open = { AnimDoor.Event_Open, "bool" },
		Lock = { AnimDoor.Event_Lock, "bool" },
		Unlock = { AnimDoor.Event_Unlock, "bool" },
		Hide = { AnimDoor.Event_Hide, "bool" },
		UnHide = { AnimDoor.Event_UnHide, "bool" },
	},
	Outputs =
	{
		Close = "bool",
		Open = "bool",
		Lock = "bool",
		Unlock = "bool",
		Hide = "bool",
		UnHide = "bool",
	},
}
