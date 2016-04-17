AIWave = {
	type = "AIWave",

	Editor =
	{
		Icon = "wave.bmp",
	},


	-- at any moment, liveAIs.size() + deadAIs.size() should be = total number of AIs in the wave.
	-- (this actually is not true at *ANY* moment: AIs can be removed/added to one of the lists "now", and then added/removed from the other one in the next frame)

	liveAIs = {}, -- liveAIs and deadAIs now store in the "value", the ID of the static entity that originated the spawn chain that created the current entity at the end.
	deadAIs = {}, --
	bookmarkAIs = {},  -- is set on level startup, and never modified after that.
	toSpawnFromDeadBookmarks = {},

	nActiveCount = 0,
	nBodyCount   = 0,
	nPoolQueueCount = 0,
	bEnableOnAdd = false,
	bIsEnabling = false,
	bIsPreparingBookmarks = false,
	bBookmarsHaveBeenPrepared = false,  -- the first time, we need to prepare all bookmarks, not just the ones in the live list
	bDisabledAndCleared = false, -- When true: the wave has been DisabledAndCleared and non pool entities have been permanently destroyed, and pool entities reseted, making the wave no longer usable
}

AIWaveBookmarkCache = {};

--################# global functions ##################################

function ExecuteAIWaveBookmarkCache(myName)
	for entityId,waveName in pairs(AIWaveBookmarkCache) do
		local wave = GetAIWaveFromName(waveName);
		if (wave and wave:GetName() == myName) then
			wave:AddBookmarked(entityId);
			AIWaveBookmarkCache[entityId] = nil;
		end
	end
end


-------------------------------------------------------------------------
-- this function is called only on level init. this, with ExecuteAIWaveBookmarkCache(), solves the problem of entity AIs created before the entity Wave.
function AddBookmarkedToWave(entityId, wave)
	local waveTbl = GetAIWaveFromName(wave);
	if (waveTbl) then
		waveTbl:AddBookmarked(entityId);
	else
		AIWaveBookmarkCache[entityId] = wave;
	end
end


-----
function GetAIWave(entity)
	local waveName = entity.AI.Wave;
	return GetAIWaveFromName(waveName);
end

-----
function GetAIWaveFromName(waveName)
	if (waveName) then
		return System.GetEntityByName(waveName);
	end
	return nil;
end


-- ########  bookmark helpers ######################################

------------------------------------------------------------------------------
-- restores into the active pool all bookmarked alive AIs  (or all of them if it is the first time)
function AIWave:PrepareBookmarkedAI()
	self.bIsPreparingBookmarks = true;

	local isFirstTime = not self.bBookmarsHaveBeenPrepared;

	if (self:GetPoolQueueCount() <= 0) then
		for entityId,v in pairs(self.bookmarkAIs) do
			local entity = System.GetEntity(entityId);
			local isInLiveList = Set.Get( self.liveAIs, entityId );
			if (not entity and (isInLiveList or isFirstTime) ) then
				if (System.PrepareEntityFromPool(entityId, true)) then -- If prepared immediatly, will call Add() during this call which will decrement pool queue count by 1
					self.nPoolQueueCount = self.nPoolQueueCount + 1;
				else
					local name = self:GetName();
					System.Warning("Input Enable of AI Wave " .. name .. " failed to prepare pooled entity");
				end
			end
		end
	end

	self.bBookmarsHaveBeenPrepared = true;
	self.bIsPreparingBookmarks = false;
end


--------------------------------------------------------------------------------
function AIWave:ResetBookmarkedAI()
	if (System.GetCVar("es_ClearPoolBookmarksOnLayerUnload") > 0) then
		for entityId,v in pairs(self.bookmarkAIs) do
			System.ResetPoolEntity(entityId);
		end
	end
end



--###########- AI Wave FG events -#####################################

---------------------------------------------------------------------------------
function AIWave:Event_Disable()
	local name = self:GetName();
	
	if (self.bDisabledAndCleared) then
		System.Warning("Wave: " .. name .. " .'Disable' input has been triggered, but the wave has already been DisabledAndCleared. Nothing will happen." );
		return;
	end

	
	if (ActiveWaves[name]) then
		local affectedTerritories = Set.New();

		-- disable all alive non-bookmarked entities
		for entityId,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityId)
			if (entity) then
				if (self:CheckAlive(entity) and entity:IsActive()) then

					if (not Set.Get( self.bookmarkAIs, entityId )) then -- if is not bookmarked, disable.
						entity:Event_Disable();
					end
					local territory = entity.AI.TerritoryShape;
					if (territory) then
						Set.Add(affectedTerritories, territory);
					end
				end
			end
		end

		self:ReturnBookmarkedAI();

		ActiveWaves[name] = false;
		BroadcastEvent(self, "Disabled");

		self:UpdateActiveCount();
		UpdateTerritoriesActiveCounts(affectedTerritories);
	end

end

-- re-bookmark the bookmarked entities
function AIWave:ReturnBookmarkedAI()
	for entityId,v in pairs(self.bookmarkAIs) do
		System.ReturnEntityToPool(entityId);
	end
end


---------------------------------------------------------------------------------
function AIWave:Event_Enable()
	local name = self:GetName();

	if (self.bDisabledAndCleared) then
		System.Warning("Wave: " .. name .. " .'Enable' input has been triggered, but the wave has already been DisabledAndCleared. Nothing will happen." );
		return;
	end

	self.bIsEnabling = true;

	if (not ActiveWaves[name]) then

		self:PrepareBookmarkedAI();

		local foundLiveAI = false
		for entityID,v in pairs(self.liveAIs) do
			foundLiveAI = true
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity)) then
					local territory = entity.AI.TerritoryShape;
					if ((not territory) or (ActiveTerritories[territory])) then
						self:TryBecomeActive(name);

						if (not entity:IsActive()) then
							self:EnableEntity(entity)
						end
					else
						if (not territory) then
							System.Warning("Input Enable of AI Wave " .. name .. " about to fail : nil territory!");
						else
							System.Warning("Input Enable of AI Wave " .. name .. " about to fail : territory " .. territory .. " is not active!");
						end
					end
				end
			end
		end

		self.bIsEnabling = false;

		if (ActiveWaves[name]) then
			self:UpdateActiveCount();
		elseif (not foundLiveAI) then
			System.Warning("Couldn't find any live AI in AI wave " .. name .. ". It will be enabled as soon as an AI is added to it.");
			self.bEnableOnAdd = true;
		end
	end

	self.bIsEnabling = false;

end


---------------------------------------------------------------------------------
function AIWave:Event_Spawn()
	local name = self:GetName();

	if (self.bDisabledAndCleared) then
		System.Warning("Wave: " .. name .. " .'Spawn' input has been triggered, but the wave has already been DisabledAndCleared. Nothing will happen." );
		return;
	end

	self.bIsEnabling = true;

	local enabled = false;
	local spawned = false;

	if (not ActiveWaves[name]) then

		self:PrepareBookmarkedAI();

		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				local territory = entity.AI.TerritoryShape;
				if ((not territory) or ActiveTerritories[entity.AI.TerritoryShape]) then
					self:TryBecomeActive(name);

					if (self:CheckAlive(entity)) then
						enabled = true;
						if (not entity:IsActive()) then
							self:EnableEntity(entity)
						end
					end
				end
			end
		end

	end


	-- restore the bookmarked ones from the deadlist. we need to do this because we cant spawn a new AI when the initial static AI is bookmarked
	self.bIsPreparingBookmarks = true;
	for entityId,staticId in pairs(self.deadAIs) do
		local entity = System.GetEntity(staticId);
		if (not entity) then
		  Set.Set( self.toSpawnFromDeadBookmarks, staticId, entityId );  -- to know which AI is the one that was actually dead, corresponding to the staticAI. we will need that info when going to spawn the new AI when Add() is called
			if ( not System.PrepareEntityFromPool(staticId)) then
				local name = self:GetName();
				System.Warning("AIWave " .. name .. " failed to prepare pooled entity: %s", tostring(staticId));
			end
			if ( not Set.Get(self.toSpawnFromDeadBookmarks, staticId)) then  -- if is already not in the list, is because System.PrepareEntityFromPool recovered the entity instantly, and a new entity has been already spawned
				spawned = true;
			end

		end
	end
	self.bIsPreparingBookmarks = false;


  -- normal respawn from dead entities
	for entityId,staticId in pairs(self.deadAIs) do
		local entity = System.GetEntity(entityId);
		local staticEntity = System.GetEntity(staticId);

-- if staticEntity is not there, is because it was bookmarked, and is in queue to be restored from bookmark. The spawn will happens when the entity is actually restored. ( Add() will be called, and e spawn will be done there). why make it simple.
		if (staticEntity) then
			local territory = entity.AI.TerritoryShape;
			if ((not territory) or (ActiveTerritories[entity.AI.TerritoryShape])) then
				self:TryBecomeActive(name);

				if (entity) then
					self:SpawnEntity(entity)
				else
					staticEntity.spawnedEntity = nil;       --it gets more and more tricky. it could be that the spawned entity was removed while the static was bookmarked, so maybe the static was never notified about it...
					staticEntity.lastSpawnedEntity = nil;
					self:SpawnEntity(staticEntity)
				end
				spawned = true;
				Set.Remove(self.deadAIs, entityId);
			end
		end
	end

	self.bIsEnabling = false;

	if (not ActiveWaves[name]) then
		System.Warning("Input Spawn of AI Wave " .. name .. " was activated but it was not enabled");
	end

	if (enabled or spawned) then
		self:UpdateActiveCount();

		-- Give AIs some time to really spawn
		Script.SetTimerForFunction(500, "DelayedBroadcastEventSpawned", self);
	end

end
function DelayedBroadcastEventSpawned(entity)
	if (entity and (type(entity) == "table")) then
		BroadcastEvent(entity, "Spawned")
	end
end

--------------------------------------------------------------------------------
function AIWave:TryBecomeActive(name)
	if (self.bIsPreparingBookmarks) then
		return;
	end

	if (not ActiveWaves[name]) then
		if (not self.nPoolQueueCount or self.nPoolQueueCount <= 0) then
			ActiveWaves[name] = true;
			BroadcastEvent(self, "Enabled");
		end
	end
end


---------------------------------------------------------------------------------
function AIWave:Event_Kill()
	local name = self:GetName();

	if (self.bDisabledAndCleared) then
		System.Warning("Wave: " .. name .. " .'Kill' input has been triggered, but the wave has already been DisabledAndCleared. Nothing will happen."  );
		return;
	end

	if (ActiveWaves[name]) then
		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if (self:CheckAlive(entity)) then
					local territory = entity.AI.TerritoryShape;
					if ((not territory) or ActiveTerritories[entity.AI.TerritoryShape]) then
						if (entity:IsActive()) then
							entity:Event_Kill();
						end
					end
				end
			end
		end
	end
end


---------------------------------------------------------------------------------------
function AIWave:Event_DisableAndClear()
	local name = self:GetName();

	if (self.bDisabledAndCleared) then
		System.Warning("Wave: ".. name .. " .'DisableAndClear' input has been triggered, but the wave has already been DisabledAndCleared. Nothing will happen." );
		return;
	end

	self:Event_Disable();
	self:ResetBookmarkedAI();
		
	-- removes all spawned entities
	for entityId,staticId in pairs(self.deadAIs) do
		local entity = System.GetEntity(entityId);
		if (entity and entity.whoSpawnedMe) then
			System.RemoveEntity( entityId );
		end
	end
	
	for entityId,staticId in pairs(self.liveAIs) do
		local entity = System.GetEntity(entityId);
		if (entity and entity.whoSpawnedMe) then
			System.RemoveEntity( entityId );
		end
	end
	
	self:Reset();
	
	self.bDisabledAndCleared = true;

end



--########### ################################---------------------

function AIWave:AllowActorRemoval(entity)
	local name = self:GetName();
	if (not ActiveWaves[name]) then
	  return true;
	end

	if (Set.Get( self.deadAIs, entity.id ) ) then -- if the corpse entity is one of the last spawned entities, dont remove. we need it for next respawn.
    return false;
	end

	return true;
end


---------------------------------------------------------------------------------------
function AIWave:OnInit()
	self.bookmarkAIs = Set.New();
	self:OnReset();
end


---------------------------------------------------------------------------------------
function AIWave:OnLoad(tbl)
	local name = self:GetName();
	ActiveWaves[name] = tbl.bIsActive;

	self.liveAIs = Set.DeserializeValues(tbl.liveAIs);
	self.deadAIs = Set.DeserializeValues(tbl.deadAIs);
	self.toSpawnFromDeadBookmarks = Set.DeserializeEntities(tbl.toSpawnFromDeadBookmarks);

	self.bookmarkAIs = Set.New();
	self.bookmarkAIs = Set.DeserializeItems(tbl.bookmarkAIs);

	self.nActiveCount = tbl.nActiveCount;
	self.nBodyCount   = tbl.nBodyCount;
	self.nPoolQueueCount = tbl.nPoolQueueCount;
	self.bEnableOnAdd = tbl.bEnableOnAdd;
	self.bIsEnabling = tbl.bIsEnabling;
	self.bIsPreparingBookmarks = tbl.bIsPreparingBookmarks;
	self.bBookmarsHaveBeenPrepared = tbl.bBookmarsHaveBeenPrepared;
	self.bDisabledAndCleared = tbl.bDisabledAndCleared;

	self.currentAssignment = tbl.currentAssignment
end

---------------------------------------------------------------------------------------
function AIWave:OnPostLoad()
-- TODO: un-duplicate code
	for entityID, staticEntityID in pairs(self.liveAIs) do
		local entity = System.GetEntity(entityID);
	  if (entity) then
	  	if (entityID ~= staticEntityID) then
	  		local staticEntity = System.GetEntity(staticEntityID);
	  	  if (staticEntity) then
	  	  	entity.whoSpawnedMe = staticEntity;
	  	  else
	  	  	entity.whoSpawnedMe = {};  -- when the staticentity is not there, is because is bookmarked. in that case we create an skeleton with the id (that is actually needed), the rest will be re-created after the next spawn
	  	  	entity.whoSpawnedMe.id = staticEntityID;
	  	  end
	  	end
		end
	end

	for entityID, staticEntityID in pairs(self.deadAIs) do
		local entity = System.GetEntity(entityID);
	  if (entity) then
	  	if (entityID ~= staticEntityID) then
	  		local staticEntity = System.GetEntity(staticEntityID);
	  	  if (staticEntity) then
	  	  	entity.whoSpawnedMe = staticEntity;
	  	  else
	  	  	entity.whoSpawnedMe = {};
	  	  	entity.whoSpawnedMe.id = staticEntityID;
	  	  end
	  	end
		end
	end
end


---------------------------------------------------------------------------------------
-- OnReset() is usually called only from the Editor, so we also need OnInit()
function AIWave:OnReset()
	self:Reset();
end

function AIWave:Reset()		
	local name = self:GetName();
	ActiveWaves[name] = false;

	self.liveAIs = Set.New();
	self.deadAIs = Set.New();
	self.toSpawnFromDeadBookmarks = Set.New();

	ExecuteAIWaveBookmarkCache(name);

	self.nActiveCount = 0;
	self.nBodyCount   = 0;
	self.nPoolQueueCount = 0;
	self.bEnableOnAdd = false;
	self.bIsEnabling = false;
	self.bIsPreparingBookmarks = false;
	self.bBookmarsHaveBeenPrepared = false;

	self.currentAssignment = nil
	self.bDisabledAndCleared = false;
	
end


---------------------------------------------------------------------------------------
function AIWave:OnSave(tbl)
	local name = self:GetName();
	tbl.bIsActive = ActiveWaves[name];
	
	tbl.liveAIs = Set.SerializeValues(self.liveAIs);
	tbl.deadAIs = Set.SerializeValues(self.deadAIs);
	tbl.toSpawnFromDeadBookmarks = Set.SerializeEntities(self.toSpawnFromDeadBookmarks);
	tbl.bookmarkAIs = Set.SerializeItems(self.bookmarkAIs);

	tbl.nActiveCount = self.nActiveCount;
	tbl.nBodyCount   = self.nBodyCount;
	tbl.nPoolQueueCount = self.nPoolQueueCount;
	tbl.bEnableOnAdd = self.bEnableOnAdd;
	tbl.bIsEnabling = self.bIsEnabling;
	tbl.bIsPreparingBookmarks = self.bIsPreparingBookmarks;
	tbl.bBookmarsHaveBeenPrepared = self.bBookmarsHaveBeenPrepared;
	tbl.bDisabledAndCleared = self.bDisabledAndCleared;

	tbl.currentAssignment = self.currentAssignment
end;


--[[
function AIWave:LogAIsTables( place )
  Log( "-@@@@@@-loging AIsTables from: %s --@@@@@@@@", place );
	for entityId,staticId in pairs(self.liveAIs) do
		Log( "-@@@@@@-live AI: %s %s", tostring(entityId), tostring(staticId) );
	end
	for entityId,staticId in pairs(self.deadAIs) do
		Log( "-@@@@@@@-dead AI: %s %s", tostring(entityId), tostring(staticId) );
	end
  Log( "------@@@----------END----------@@@--------", place );
  Log( " " );
end
--]]

-------------------------------------------------------------------------------------
function AIWave:OnHidden()
	self:ResetBookmarkedAI();
end


--######### AI Wave private methods ############################


------------------------------------------------------------------
-- returns the ID of the static entity that spawned this one
function AIWave:GetStaticEntityID( entity )
	if (entity.whoSpawnedMe) then
		return entity.whoSpawnedMe.id;
  else
  	return entity.id;
  end
end



------------------------------------------------------------------------
-- this function is called when a new AI is spawned
function AIWave:Add(entity)
	local name = self:GetName();
	if (name == entity.AI.Wave) then
		local staticID = self:GetStaticEntityID( entity );

		if (Set.Get(self.bookmarkAIs, entity.id)) then
			if ( not Set.Get(self.toSpawnFromDeadBookmarks, entity.id )) then -- this should always be true (as not being in the list) anyway, adding just for extra precaution
				Set.Add( self.liveAIs, entity.id, staticID );
				self:AddFromBookmark(entity);
			end
		else
			Set.Add( self.liveAIs, entity.id, staticID );
			self:AddNormal(entity);
		end
	end
end

------------------------------------------------------------------------
function AIWave:AddNormal(entity)
	if (entity:IsActive()) then
		local name = self:GetName();
		local territory = entity.AI.TerritoryShape;
		local AICanBeActivated = ((not territory) or ActiveTerritories[territory]);

		if (AICanBeActivated) then
			if (not ActiveWaves[name]) then
				if (self.bEnableOnAdd) then
					self:EnableEntity(entity)
				end
			end
			self.bEnableOnAdd = false;

			if (ActiveWaves[name]) then
				self:UpdateActiveCount();
				return;
			end
	  end
	end
	entity:Event_Disable(); -- this only happens when the AIWave was not active and for some reason could not be enabled
end

------------------------------------------------------------------------
function AIWave:AddFromBookmark(entity)
	self.nPoolQueueCount = self.nPoolQueueCount - 1;
	local name = self:GetName();
	local territory = entity.AI.TerritoryShape;
	if ((not territory) or (ActiveTerritories[territory])) then
		entity:Activate(1);
		self:EnableEntity(entity)

		self:TryBecomeActive(name);
		self:UpdateActiveCount();
	end
end

------------------------------------------------------------------------
-- called only on level start, when bookmarks are created
function AIWave:AddBookmarked(entityId)
	Set.Add(self.bookmarkAIs, entityId);
end

-----------------------------------------------------------------------
function AIWave:OnEntityPreparedFromPool( entity )
	local entityIdToSpawnFrom = Set.Get(self.toSpawnFromDeadBookmarks, entity.id );

	-- we were waiting for this static entity (entityId) to come back from bookmarked state, to spawn a new entity from the dead one (which is not the static one, but the last one spawned from it)
	if (entityIdToSpawnFrom) then
		local deadEntity = System.GetEntity(entityIdToSpawnFrom)
		if (deadEntity) then -- this should always be true
			if (deadEntity.whoSpawnedMe) then
				deadEntity.whoSpawnedMe = entity;  -- while the static entity was bookmarked, the dynamic entity kept the table, but now it needs to use the new one
			end
			entity.spawnedEntity = nil;
			self:SpawnEntity(deadEntity)
		end
		Set.Remove(self.deadAIs, entityIdToSpawnFrom);
		Set.Remove(self.toSpawnFromDeadBookmarks, entity.id );
	end
end


------------------------------------------------------------------------
-- called from CheckAlive, and also from the territory when the AI dies.
function AIWave:NotifyDeath(entity)
	if (Set.Get(self.liveAIs, entity.id)) then
	  local staticID = self:GetStaticEntityID( entity );
		Set.Remove(self.liveAIs, entity.id);
		Set.Add(self.deadAIs, entity.id, staticID);

		self:UpdateActiveCount(true);	-- Check if the wave is dead
		self.nBodyCount = self.nBodyCount + 1;
		self:ActivateOutput("BodyCount", self.nBodyCount);
	end
end

------------------------------------------------------------------------
function AIWave:IsEnabling()
	return self.bIsEnabling;
end

------------------------------------------------------------------------
function AIWave:GetPoolQueueCount()
	if (self.nPoolQueueCount >= 0) then
		return self.nPoolQueueCount;
	end

	return 0;
end

------------------------------------------------------------------------
function AIWave:GetActiveCount()
	return self.nActiveCount;
end

------------------------------------------------------------------------
function AIWave:UpdateActiveCount(bCheckIfDead)
	-- Only output the active count when we're doing becomming enabled
	if (self:IsEnabling()) then
		return;
	end

	local name = self:GetName();
	local activeCount = self:GetPoolQueueCount();
	local affectedTerritories = Set.New();


	if (ActiveWaves[name]) then
		for entityID,v in pairs(self.liveAIs) do
			local entity = System.GetEntity(entityID)
			if (entity) then
				if ((not entity:IsDead()) and entity:IsActive()) then
					activeCount = activeCount + 1;
				end
				local territory = entity.AI.TerritoryShape;
				if (territory) then
					Set.Add(affectedTerritories, territory);
				end
			end
		end
	end


	if (activeCount ~= self.nActiveCount) then
		self.nActiveCount = activeCount;
		if (self:GetPoolQueueCount()==0) then  -- we dont activate the output if there are entities waiting to be retrieved from the pool, to avoid many activations of the output in consecutive frames.
			self:ActivateOutput("ActiveCount", self.nActiveCount);
			UpdateTerritoriesActiveCounts(affectedTerritories);
		end

		--System.Log("Wave " .. activeCount);

		if (bCheckIfDead and (self.nActiveCount == 0)) then
			BroadcastEvent(self, "Dead");
		end
	end
end

------------------------------------------------------------------------
function AIWave:CheckAlive(entity)
	if (entity:IsDead()) then
		self:NotifyDeath(entity);
		return false;
	end
	return true;
end

------------------------------------------------------------------------

function AIWave.EnableEntity(wave, entity)
	entity:Event_Enable()

	if (wave.currentAssignment) then
		wave:IndividualDispatchSetAssignment(entity)
	end
end

function AIWave.SpawnEntity(wave, entity)
	entity:Event_Spawn()

	if (wave.currentAssignment) then
		local spawnedEntityId = nil;
		if (entity.whoSpawnedMe) then
			spawnedEntityId = entity.whoSpawnedMe.lastSpawnedEntity
		else
			spawnedEntityId = entity.lastSpawnedEntity
		end

		if (spawnedEntityId) then
			local spawnedEntity = System.GetEntity(spawnedEntityId)
			wave:IndividualDispatchSetAssignment(spawnedEntity)
		end
	end
end

function AIWave.SetAssignment(wave, assignment, data)
	wave:ClearAssignment()

	wave.currentAssignment = {}
	wave.currentAssignment.assignment = assignment
	wave.currentAssignment.data = {}
	if (data ~= nil) then
		mergef(wave.currentAssignment.data, data, 1)
	end

	--Log("AIWave - SetAssignment")
	wave:DispatchSetAssignment()
end

function AIWave.ClearAssignment(wave)
	wave.currentAssignment = nil
	--Log("AIWave - ClearAssignment")
	wave:DispatchClearAssignment()
end

function AIWave.DispatchSetAssignment(wave)
	for entityId,v in pairs(wave.liveAIs) do
		local entity = System.GetEntity(entityId)
		if (entity and entity:IsActive() and entity.SetAssignment) then
			entity:SetAssignment(wave.currentAssignment.assignment, wave.currentAssignment.data)
		end
	end
end

function AIWave.DispatchClearAssignment(wave)
	for entityId,v in pairs(wave.liveAIs) do
		local entity = System.GetEntity(entityId)
		if (entity and entity:IsActive() and entity.ClearAssignment) then
			entity:ClearAssignment()
		end
	end
end

function AIWave.IndividualDispatchSetAssignment(wave, entity)
	if (entity and entity.SetAssignment) then
		entity:SetAssignment(wave.currentAssignment.assignment, wave.currentAssignment.data)
	end
end

function AIWave.Event_SetFactionOfAllAIs(self, sender, faction)
	for entityId,v in pairs(self.liveAIs) do
		AI.SetFactionOf(entityId, faction);
	end
	for entityId,v in pairs(self.deadAIs) do
		AI.SetFactionOf(entityId, faction);
	end
end

------------ AI Wave FG node ------------------------------------

AIWave.FlowEvents =
{
	Inputs =
	{
		Disable = { AIWave.Event_Disable, "bool" },
		Enable  = { AIWave.Event_Enable,  "bool" },
		Kill    = { AIWave.Event_Kill,    "bool" },
		Spawn   = { AIWave.Event_Spawn,   "bool" },

		DisableAndClear = { AIWave.Event_DisableAndClear, "bool" },
		SetFactionOfAllAIs = { AIWave.Event_SetFactionOfAllAIs, "string" },
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
