GameEvent =
{
    Properties = { 
			nId = 0,
			nAllowedDeaths = 3,
			sUniqueName = "",
			bRespawnAtTagpoint=0,
		},

    Editor = { Model="Objects/Editor/Anchor.cgf", },
}


function GameEvent:OnInit()
	--self.soundobj = Sound.LoadSound("sounds/waypoint/waypoint1.wav");
end

function GameEvent:OnShutDown()
end

function GameEvent:Event_Save(sender)
	--printf("CHECKPOINT REACHED: "..self.Properties.nId);
	--if sender~=nil then
	    --System.RemoveEntity(sender.id);
	--end

	self:EnableSave(nil);

--	if (_localplayer) then
--    	_localplayer:PlaySound(self.soundobj);
--	end

    --Hud:AddMessage("Checkpoint "..self.Properties.nId.." reached");
	System.Log("Checkpoint "..self.Properties.nId.." reached");
	--Hud:AddMessage("@Checkpointreached");

	self.bSaveNow = nil;
	
	self:SetTimer(100);
end

function GameEvent:OnTimer()


	if (_localplayer.timetodie) then return end

	if (self.bSaveNow) then	
		
		self.bSaveNow = nil;
		-- if in the vehicle - can't save at respawnPoint pos/angles  
		if (self.Properties.bRespawnAtTagpoint==1 and _localplayer.theVehicle==nil ) then
			_LastCheckPPos = new (self:GetPos());
			_LastCheckPAngles = new(self:GetAngles());
		else
			_LastCheckPPos = new (_localplayer:GetPos());
			_LastCheckPAngles = new(_localplayer:GetAngles(1));
		end

		self:KillTimer();

		AI:Checkpoint();
	
		if (self.Properties.sUniqueName ~= "") then 
			if (ALLOWED_DEATHS) then 
				if (ALLOWED_DEATHS[self.Properties.sUniqueName]) then
					AI:SetAllowedDeathCount(ALLOWED_DEATHS[self.Properties.sUniqueName].deaths);
				end
			end
		else
			AI:SetAllowedDeathCount(self.Properties.nAllowedDeaths);
		end

		Game:TouchCheckPoint(self.Properties.nId, _LastCheckPPos, _LastCheckPAngles);

		if (Game.OnAfterSave) then
			Game:OnAfterSave();
		end
	else
		if (Game.OnBeforeSave) then
			Game:OnBeforeSave();
		end

		self.bSaveNow = 1;
		self:SetTimer(1);
	end
end

GameEvent.FlowEvents =
{
	Inputs =
	{
		Save = { GameEvent.Event_Save, "bool" },
	},
	Outputs =
	{
		Save = "bool",
	},
}
