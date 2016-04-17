Script.ReloadScript("Scripts/Entities/AI/AIWave.lua");

ActiveTerritories = {};
ActiveWaves       = {};

AITerritory = {
	type = "AITerritory",

	Editor =
	{
		Icon = "territory.bmp",
	},
	
	liveAIs = {},
	deadAIs = {},
	activeWaves = {}, -- Waves which currently have living AI
	historyWaves = {}, -- Waves which have been used since last reset
	
	nActiveCount = 0,
	nBodyCount   = 0,
}


------------ AI Territories & Waves global functions ------------

function AddToTerritoryAndWave(entity)
	local wave = GetAIWave(entity);
	if (wave) then
		wave:Add(entity);
	end
	local territory = GetAITerritory(entity);
	if (territory) then
		territory:Add(entity);
	end
end


-- Called from BasicAI.OnDeath(entity)
function NotifyDeathToTerritoryAndWave(entity)
	Script.SetTimerForFunction(3000, "DeferredNotifyDeathToTerritoryAndWave", entity);
end

function DeferredNotifyDeathToTerritoryAndWave(entity)
	if (entity and (type(entity) == "table")) then
		local wave = GetAIWave(entity);
		if (wave) then
			wave:NotifyDeath(entity);
		end
		local territory = GetAITerritory(entity);
		if (territory) then
			territory:NotifyDeath(entity);
		end
	end
end



------------ AI Territories & Waves helper functions ------------

function GetAITerritory(entity)
	local territoryName = entity.AI.TerritoryShape;
	return GetAITerritoryFromName(territoryName);
end


function GetAITerritoryFromName(territoryName)
	if (territoryName) then
		return System.GetEntityByName(territoryName);
	end
	return nil;
end


function UpdateTerritoriesActiveCounts(territories)
	for territoryName,v in pairs(territories) do
		local territory = System.GetEntityByName(territoryName);
		if (territory) then
			territory:UpdateActiveCount();
		end
	end
end


function AITerritory:IsWaveEnabling()
	for entityID,v in pairs(self.activeWaves) do
		local wave = System.GetEntity(entityID);
		if (wave and wave:IsEnabling()) then
			return true;
		end
	end
	
	return false;
end


function AITerritory:UpdateWavesActiveCounts(bCheckIfDead)
	local removeActiveWaves = Set.New();

	local poolQueueCount = 0;
	for entityID,v in pairs(self.activeWaves) do
		local wave = System.GetEntity(entityID);
		if (wave) then
			wave:UpdateActiveCount(bCheckIfDead);
			poolQueueCount = poolQueueCount + wave:GetPoolQueueCount();
			
			if (wave:GetActiveCount() <= 0) then
				Set.Add(removeActiveWaves, entityID);
			end
		end
	end
	
	for entityID,v in pairs(removeActiveWaves) do
		Set.Remove(self.activeWaves, entityID);
	end
	
	return poolQueueCount;
end


function AITerritory:ReturnWavesBookmarkedAI(waves)
	for entityID,v in pairs(self.activeWaves) do
		local wave = System.GetEntity(entityID);
		if (wave and wave.ReturnBookmarkedAI) then
			wave:ReturnBookmarkedAI();
		end
	end
end


function AITerritory:ResetWavesBookmarkedAI(waves)
	for entityID,v in pairs(self.historyWaves) do
		local wave = System.GetEntity(entityID);
		if (wave and wave.ResetBookmarkedAI) then
			wave:ResetBookmarkedAI();
		end
	end
end


function AITerritory:PrepareWavesBookmarkedAI()
	for entityID,v in pairs(self.historyWaves) do
		local wave = System.GetEntity(entityID);
		local waveIsActive = ActiveWaves[wave.GetName()];
		if (wave and wave.PrepareBookmarkedAI and waveIsActive) then
			wave:PrepareBookmarkedAI();
		end
	end
end

------------ AI Territory FG events -----------------------------

function AITerritory:Event_Disable()
	local name = self:GetName();
	if (ActiveTerritories[name]) then
		
		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity) and entity:IsActive()) then
					entity:Event_Disable();
				end
			end
		end
	
		self:ReturnWavesBookmarkedAI();

		ActiveTerritories[name] = false;
		BroadcastEvent(self, "Disabled");

		self:UpdateActiveCount();
	end
end


function AITerritory:Event_Enable()
	local name = self:GetName();
	if (not ActiveTerritories[name]) then
		ActiveTerritories[name] = true;

		self:PrepareWavesBookmarkedAI()

		Script.SetTimerForFunction(500, "DelayedEvent_Enable", self);

		self:UpdateActiveCount();
	end
end


function DelayedEvent_Enable(self)
	if (self and (type(self) == "table")) then
		BroadcastEvent(self, "Enabled");

		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity)) then
					local wave = entity.AI.Wave;
					if (((not wave) or (ActiveWaves[wave]))  and (not entity:IsActive())) then
						entity:Event_Enable();
					end
				end
			end
		end
	end
end


function AITerritory:Event_Kill()
	local name = self:GetName();
	if (ActiveTerritories[name]) then
		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity) and entity:IsActive()) then
					entity:Event_Kill();
				end
			end
		end
	end
end


function AITerritory:Event_Spawn()
	local enabled = false;
	local spawned = false;

	local name = self:GetName();
	if (not ActiveTerritories[name]) then
		ActiveTerritories[name] = true;
		BroadcastEvent(self, "Enabled");

		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity)) then
					local wave = entity.AI.Wave;
					if (((not wave) or (ActiveWaves[wave]))  and (not entity:IsActive())) then
						entity:Event_Enable();
						enabled = true;
					end
				end
			end
		end
	end
	
	for entityID,v in pairs(self.deadAIs) do
		local entity = System.GetEntity(entityID)
		if (entity) then
			if (not entity.AI.Wave) then
				-- Clone dead cloned entity only IF ITS ORIGINAL IS DETACHED FROM IT
				if ((not entity.whoSpawnedMe) or (not entity.whoSpawnedMe.spawnedEntity)) then
					entity:Event_Spawn();
					spawned = true;
					Set.Remove(self.deadAIs, entity.id);
				end
			end
		end
	end
	
	if (enabled or spawned) then
		self:UpdateActiveCount();
		
		-- Give AIs some time to really spawn
		Script.SetTimerForFunction(500, "DelayedBroadcastEventSpawned", self);
	end
end


function DelayedBroadcastEventSpawned(entity)
	if (entity and (type(entity) == "table")) then
		BroadcastEvent(entity, "Spawned");
	end
end


function AITerritory:Event_DisableAndClear()
	local name = self:GetName();
	
	for entityID,v in pairs(self.historyWaves) do
		local wave = System.GetEntity(entityID);
		if (wave) then
			wave:Event_DisableAndClear();
		end
	end
	
	ActiveTerritories[name] = false;
	BroadcastEvent(self, "Disabled");

	self:UpdateActiveCount();
end


------------ AI Territory callbacks -----------------------------

function AITerritory:OnInit()
	self:OnReset();
end


function AITerritory:OnLoad(tbl)
	local name = self:GetName();
	ActiveTerritories[name] = tbl.bIsActive;
	
	self.liveAIs = Set.DeserializeEntities(tbl.liveAIs);
	self.deadAIs = Set.DeserializeEntities(tbl.deadAIs);
	self.activeWaves = Set.DeserializeEntities(tbl.activeWaves);
	self.historyWaves = Set.DeserializeEntities(tbl.historyWaves);
	
	self.nActiveCount = tbl.nActiveCount;
	self.nBodyCount   = tbl.nBodyCount;
end


-- OnReset() is usually called only from the Editor, so we also need OnInit()
function AITerritory:OnReset()
	local name = self:GetName();
	ActiveTerritories[name] = false;
	
	self.liveAIs = Set.New();
	self.deadAIs = Set.New();
	self.activeWaves = Set.New();
	self.historyWaves = Set.New();
	
	self.nActiveCount = 0;
	self.nBodyCount   = 0;
end


function AITerritory:OnSave(tbl)
	local name = self:GetName();
	tbl.bIsActive = ActiveTerritories[name];
	
	tbl.liveAIs = Set.SerializeEntities(self.liveAIs);
	tbl.deadAIs = Set.SerializeEntities(self.deadAIs);
	tbl.activeWaves = Set.SerializeEntities(self.activeWaves);
	tbl.historyWaves = Set.SerializeEntities(self.historyWaves);
	
	tbl.nActiveCount = self.nActiveCount;
	tbl.nBodyCount   = self.nBodyCount;
end



------------ AI Territory private methods -----------------------

function AITerritory:Add(entity)
	local name = self:GetName();
	if (name == entity.AI.TerritoryShape) then
		
		local waveName = entity.AI.Wave;
		if (waveName) then
			local wave = System.GetEntityByName(waveName);
			if (wave) then
				Set.Add(self.activeWaves, wave.id);
				Set.Add(self.historyWaves, wave.id);
			end
		end
		
		if (entity:IsDead()) then
			Set.Add(self.deadAIs, entity.id);
		else
		  local isActuallyDead = Set.Get(self.deadAIs, entity.id); -- IsDead() will fail for dead entities that are being constructed from a bookmark
		  if (not isActuallyDead) then
				Set.Add(self.liveAIs, entity.id);
				if (entity:IsActive() and ActiveTerritories[name]) then
					self:UpdateActiveCount();
				else
					entity:Event_Disable();
				end
			end
		end
		
	end
end


function AITerritory:NotifyDeath(entity)
	if (Set.Get(self.liveAIs, entity.id)) then
		Set.Remove(self.liveAIs, entity.id);
		Set.Add(self.deadAIs, entity.id);
		
		self:UpdateActiveCount(true);	-- Check if the territory is dead
		self.nBodyCount = self.nBodyCount + 1;
		self:ActivateOutput("BodyCount", self.nBodyCount);
	end
end

--function AITerritory:LogAIsTables( place )
--	for entityId,staticId in pairs(self.liveAIs) do
--		Log( "-######-live AI: %s", tostring(entityId) );
--	end
--	for entityId,staticId in pairs(self.deadAIs) do
--		Log( "-#######-dead AI: %s", tostring(entityId) );
--	end
--  Log( "------###----------END----------###--------", place );
--end


function WaveAllowActorRemoval(entity)
	local wave = GetAIWave(entity);
	if (wave) then
		return wave:AllowActorRemoval(entity)
	else
	  return true;
	end
end


function AITerritory:UpdateActiveCount(bCheckIfDead)
	-- Only output the active count when all waves are not in the process of being enabled
	if (self:IsWaveEnabling()) then
		return;
	end
	
	local name = self:GetName();
	local activeCount = self:UpdateWavesActiveCounts(bCheckIfDead);

	if (ActiveTerritories[name]) then
		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity) and entity:IsActive()) then
					activeCount = activeCount + 1;
				end
			end
		end
	end
		
	if (activeCount ~= self.nActiveCount) then
		self.nActiveCount = activeCount;
		self:ActivateOutput("ActiveCount", self.nActiveCount);
		
		--System.Log("Territory " .. activeCount);
		
		if (bCheckIfDead and (self.nActiveCount == 0)) then
			BroadcastEvent(self, "Dead");
		end
	end
end


function AITerritory:CheckAlive(entity)
	if (entity:IsDead()) then
		self:NotifyDeath(entity);
		return false;
	end
	return true;
end


function AITerritory:OnHidden()
	self:ResetWavesBookmarkedAI();
end



------------ AI Territory FG node -------------------------------

AITerritory.FlowEvents =
{
	Inputs =
	{
		Disable = { AITerritory.Event_Disable, "bool" },
		Enable  = { AITerritory.Event_Enable,  "bool" },
		Kill    = { AITerritory.Event_Kill,    "bool" },
		Spawn   = { AITerritory.Event_Spawn,   "bool" },
		
		DisableAndClear = { AITerritory.Event_DisableAndClear, "bool" },
	},

	Outputs =
	{
		ActiveCount = "int",
		BodyCount   = "int",
		Dead        = "bool",
		Disabled    = "bool",
		Enabled     = "bool",
		Spawned     = "bool",
	},
}
