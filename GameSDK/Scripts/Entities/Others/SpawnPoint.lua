--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Spawn Point
--  
--------------------------------------------------------------------------
--  History:
--  - 24:9:2004   12:00 : Created by Mathieu Pinard
--
--------------------------------------------------------------------------

SpawnPoint = {
	Client = {},
	Server = {},

	Editor={
		Model="Editor/Objects/spawnpointhelper.cgf",
		Icon="SpawnPoint.bmp",
		DisplayArrow=1,
	},
	
	Properties=
	{
		teamName="",
		groupName="",
		bEnabled=1,
		bInitialSpawn=0,
		bDoVisTest=1,
	},
}

--------------------------------------------------------------------------
function SpawnPoint.Server:OnInit()
	if (System.IsMultiplayer()) then
		g_gameRules.game:AddSpawnLocation(self.id, self.Properties.bInitialSpawn, self.Properties.bDoVisTest, self.Properties.groupName);
	end

	self:Enable(tonumber(self.Properties.bEnabled)~=0);	
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	g_gameRules.game:ClientSetTeam(g_gameRules.game:GetTeamId(self.Properties.teamName) or 0, self.id);

	if (System.IsEditor()) then
		self:LoadObject(0, self.Editor.Model);
	end

end

function SpawnPoint:OnEditorSetGameMode(gameMode)
	if (gameMode) then
		self:DrawSlot(0, 0);
	else
		self:DrawSlot(0, 1);
	end
end


--------------------------------------------------------------------------
function SpawnPoint.Client:OnInit()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	if (not CryAction.IsServer()) then
		g_gameRules.game:AddSpawnLocation(self.id, self.Properties.bInitialSpawn, self.Properties.bDoVisTest, self.Properties.groupName);
		g_gameRules.game:ClientSetTeam(g_gameRules.game:GetTeamId(self.Properties.teamName) or 0, self.id);
		self:Enable(tonumber(self.Properties.bEnabled)~=0);
	end
end

----------------------------------------------------------------------------------------------------
function SpawnPoint:Enable(enable)
	-- Log("spawn point %s enable %s teamname %s", self:GetName(), tostring(enable), self.Properties.teamName);
	if (System.IsMultiplayer()) then
		if (enable) then
			g_gameRules.game:EnableSpawnLocation(self.id, self.Properties.bInitialSpawn, self.Properties.groupName);
		else
			g_gameRules.game:DisableSpawnLocation(self.id, self.Properties.bInitialSpawn);
		end
	else
		if (enable) then
			g_gameRules.game:AddSpawnLocation(self.id, self.Properties.bInitialSpawn, self.Properties.bDoVisTest, self.Properties.groupName);
		else
			g_gameRules.game:RemoveSpawnLocation(self.id, self.Properties.bInitialSpawn);
		end
	end
	self.enabled=enable;
end

--------------------------------------------------------------------------
function SpawnPoint:OnPropertyChange()
	self:Enable(tonumber(self.Properties.bEnabled)~=0);
end

--------------------------------------------------------------------------
function SpawnPoint.Server:OnShutDown()
	if (g_gameRules) then
		g_gameRules.game:RemoveSpawnLocation(self.id, self.Properties.bInitialSpawn);
	end
end

--------------------------------------------------------------------------
function SpawnPoint.Client:OnShutDown()
	if (g_gameRules) and (not CryAction.IsServer()) then
		g_gameRules.game:RemoveSpawnLocation(self.id, self.Properties.bInitialSpawn);
	end
end

--------------------------------------------------------------------------
function SpawnPoint:Spawned(entity)
	self:ActivateOutput("Spawn", entity.id);
end

--------------------------------------------------------------------------
function SpawnPoint:IsEnabled()
	return self.enabled;
end

--------------------------------------------------------------------------
-- Event is generated when something is spawned using this spawnpoint
--------------------------------------------------------------------------
function SpawnPoint:Event_Spawn()
	
	if(g_localActor == nil)then
		return;
	end
	
	local player = g_localActor;
	player:SetWorldPos(self:GetWorldPos(g_Vectors.temp_v1));		
	
	self:GetAngles(g_Vectors.temp_v1);
	g_Vectors.temp_v1.x = 0;
	g_Vectors.temp_v1.y = 0;
	player.actor:PlayerSetViewAngles( g_Vectors.temp_v1 );
	
	self:ActivateOutput("Spawn", player.id);
end


function SpawnPoint:Event_Enable()
	self:Enable(true);
	BroadcastEvent(self, "Enabled");
end

function SpawnPoint:Event_Disable()
	self:Enable(false);
	BroadcastEvent(self, "Disabled");
end

SpawnPoint.FlowEvents =
{
	Inputs =
	{
		Spawn = { SpawnPoint.Event_Spawn, "bool" },
		Enable = { SpawnPoint.Event_Enable, "bool" },
		Disable = { SpawnPoint.Event_Disable, "bool" },
	},
	Outputs =
	{
		Spawn = "entity",
		Enabled = "bool",
		Disabled = "bool",
	},
}
