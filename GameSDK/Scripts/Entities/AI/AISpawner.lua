--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--
--	Description: AI auto spawner 
--  
--------------------------------------------------------------------------
--  History:
--  - 10:11:2006   12:00 : Created by Kirill
--
--------------------------------------------------------------------------

AISpawner = {
	Client = {},
	Server = {},

	Editor={
		Model="Editor/Objects/Particles.cgf",
		Icon="SpawnPoint.bmp",
		DisplayArrow=1,
	},
	
	Properties=
	{
--		SpawnDelay = 1,
		NumUnits = 2,
		Limit = 4,
		bLimitStop = 1,
		bDoVisCheck = 0,
	},
	
	unitsCounter = 0,				-- units currently alive
	totalUnitsCounter = 0,	-- total number of spawned units
	isEnabled=0,
	
	visDummys = {},
	spawnedIds = {},	-- vector of IDs of spawned entities
	spawnedIdsSize=0, -- size of the vector above
}

--------------------------------------------------------------------------
function AISpawner.Server:OnInit()

AI.LogEvent("AISpawner.Server:OnInit >>>");

	self:OnReset();	
	self:CreateDummy();
	
end

----------------------------------------------------------------------------------------------------
function AISpawner.Server:OnShutDown()

	self:RemoveDummy();
	
end

-------------------------------------------------------
function AISpawner:OnPropertyChange()

	self:CreateDummy();
	
end


----------------------------------------------------------------------------------------------------


--------------------------------------------------------------------------
function AISpawner:OnReset()

	self.unitsCounter = 0;
	self.totalUnitsCounter = 0;
	self.isEnabled = 0;

	self.spawnedIds = {};
	self.spawnedIdsSize=0;
end

----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
function AISpawner:OnSave(save)

	save.unitsCounter = self.unitsCounter;
	save.totalUnitsCounter = self.totalUnitsCounter;
	save.isEnabled = self.isEnabled;
	
	save.spawnedIds = self.spawnedIds;
	save.spawnedIdsSize = self.spawnedIdsSize;
end


----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
function AISpawner:OnLoad(saved)

	self.unitsCounter = saved.unitsCounter;
	self.totalUnitsCounter = saved.totalUnitsCounter;
	self.isEnabled = saved.isEnabled;
	
	self.spawnedIds = saved.spawnedIds;
	self.spawnedIdsSize = saved.spawnedIdsSize;
	
end

----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
function AISpawner:UnitDown()

--AI.LogEvent(" >>>Unit down >> units left  "..self.unitsCounter.."  |>> spawn number "..self.Properties.NumUnits);
	self.unitsCounter = self.unitsCounter-1;
	
	if(self.isEnabled == 0) then return end
	
	if(self.unitsCounter < self.Properties.NumUnits) then
		self:SpawnInitially();
	end

end

----------------------------------------------------------------------------------------------------
function AISpawner:SpawnUnit(id)

--AI.LogEvent(" >>>Spawning unit : counter is "..self.unitsCounter);

--	local link=self:GetLink(idx);
	local link = System.GetEntity(id);
	if( link==nil ) then return end
	
	link.spawnedEntity = nil;
	if (link.Event_SpawnKeep) then
		link:Event_SpawnKeep();	
	elseif(link.SpawnCopyAndLoad) then
		link:SpawnCopyAndLoad();
	end
	
	if(link.spawnedEntity) then
		local newEntity = System.GetEntity(link.spawnedEntity);
--		self.spawnedEntity = 0;
		if(newEntity) then

			if(link.PropertiesInstance.bAutoDisable ~= 1) then
				AI.AutoDisable( newEntity.id, 0 );
			end
			if(newEntity.class=="Scout") then -- will check with Kirill later. Tetsuji
				AI.AutoDisable( newEntity.id, 0 );
			end

			newEntity.AI.spawnerListenerId = self.id;
			self.unitsCounter = self.unitsCounter+1;
			self.totalUnitsCounter = self.totalUnitsCounter+1;
			self:FindSpawnReinfPoint();
			newEntity.AI.reinfPoint = g_SignalData.ObjectName;
			newEntity:SetName(newEntity:GetName().."_spawned");
			AI.Signal(SIGNALFILTER_SENDER,0,"NEW_SPAWN",newEntity.id,g_SignalData);
	--			newEntity:SelectPipe(0,"goto_point","REINF");
			self.spawnedIds[self.spawnedIdsSize] = newEntity.id;
			self.spawnedIdsSize = self.spawnedIdsSize+1;
		end
	end
end

----------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------
function AISpawner:FindSpawnProtoUnitId()
--AI.LogEvent(" >>>FindSpawnProtoUnitIdx ");
		
		local linkCount = self:CountLinks();
		if(linkCount < 1) then return 0	end

		if(self.Properties.bDoVisCheck == 0) then
			while (1) do
				local spawnIdx = random(1, linkCount)-1;
				local link = self:GetLink(spawnIdx);
				if (link) then
					AI.LogEvent(" >>>FindSpawnProtoUnitIdx checking "..spawnIdx);
					if(AI.GetTypeOf( link.id ) ~= AIOBJECT_WAYPOINT) then
						return link.id;
					end
				end

				if (linkCount == 1) then
					return;
				end
			end
		else
			for protoId, dummy in pairs(self.visDummys) do
				if(dummy:UnSeenFrames()>100)then
					return protoId
				end
			end
		end

		return 0 


--	while (1) do
--		local spawnIdx = random(1,self:CountLinks())-1;
--		local link=self:GetLink(spawnIdx);
--AI.LogEvent(" >>>FindSpawnProtoUnitIdx checking "..spawnIdx);
--		if(AI.GetTypeOf( link.id ) ~= AIOBJECT_WAYPOINT) then
--			return spawnIdx;		
--		end
--	end
end

----------------------------------------------------------------------------------------------------
-- find reinf point closest to teh target (?? g_localActor ??)
function AISpawner:FindSpawnReinfPoint()
--AI.LogEvent(" >>>FindSpawnReinfPoint");

	Log("FindSpawnReinfPoint")
	local targetPos=self:GetPos();
	if(g_localActor) then
		targetPos=g_localActor:GetPos();
	end	
	local minDistSq=100000000;
	local bestIdx=0;
	local i=0;
	local link=self:GetLink(i);
	while (link) do
		if(AI.GetTypeOf( link.id ) == AIOBJECT_WAYPOINT) then
			local distSq=DistanceSqVectors( targetPos, link:GetPos() );
			if(distSq < minDistSq) then
				minDistSq = distSq;
				bestIdx = i;
			end
		end	
		i=i+1;
		link=self:GetLink(i);
	end
	link=self:GetLink(bestIdx);
	g_SignalData.ObjectName = link:GetName();
	Log("FindSpawnReinfPoint end")
end

----------------------------------------------------------------------------------------------------
function AISpawner:SpawnInitially()
	Log("SpawnInitially")
--AI.LogEvent(" >>>Spawning initially : NumUnits is "..self.Properties.NumUnits);
	while (self.unitsCounter < self.Properties.NumUnits) do
		-- check the limits, make sure not to go over	
		if(self.totalUnitsCounter == self.Properties.Limit) then	
			self:Event_Limit();
			-- might get desabled by Limit
			if(self.isEnabled == 0) then return end
		end
	
		local spawnId = self:FindSpawnProtoUnitId(); --random(1,self:CountLinks())-1;
		if(spawnId == 0) then
			AI.LogEvent(" >>>Spawning initially : Can't find valid spawn proto/point");
			return
		end
		self:SpawnUnit(spawnId);
	end
	Log("SpawnInitially end")
end

----------------------------------------------------------------------------------------------------
function AISpawner:RemoveDummy()

	for protoId, dummy in pairs(self.visDummys) do
		System.RemoveEntity(dummy.id)
	end
	self.visDummys = {};
end


----------------------------------------------------------------------------------------------------
function AISpawner:CreateDummy()

	Log("createdummy start")
	self:RemoveDummy();

	if(self.Properties.bDoVisCheck == 0) then return end
	
	local i=0;
	local link=self:GetLink(i);
	while (link) do
		if(AI.GetTypeOf( link.id ) ~= AIOBJECT_WAYPOINT) then
			local dummyEntity = self;	
			local params = {
				class = "Dummy";
				position = link:GetPos(),
	--			orientation = self:GetDirectionVector(1),
	--			scale = self:GetScale(),
	--			properties = self.Properties,
	--			propertiesInstance = self.PropertiesInstance,
			}
			params.name = link:GetName().."_VisDummy"
			dummyEntity = System.SpawnEntity(params)
			dummyEntity:LoadObject(0, "objects/box_nodraw.cgf");
--		ent:DrawSlot(0, 1);
--		ent:SetLocalBBox()
			local bbmin,bbmax = link:GetLocalBBox();
			dummyEntity:SetLocalBBox(bbmin,bbmax);
			self.visDummys[link.id] = dummyEntity;
		end
		i = i + 1;		
		link=self:GetLink(i);
	end
	Log("createdummy end")
end




--------------------------------------------------------------------------
-- Event is generated when something is spawned using this spawnpoint
--------------------------------------------------------------------------
--function AISpawner:Event_Spawn(params)
--
--	while (self.unitsCounter < self.Properties.NumUnits) do
--		local spawnIdx = random(1,self:CountLinks())-1;
--AI.LogEvent(" >>>AISpawner:Event_TestSpawn : idx "..spawnIdx);
--		self:SpawnUnit(spawnIdx);
--	end
--				
--	BroadcastEvent(self, "Spawn");
--end

--------------------------------------------------------------------------
function AISpawner:Event_Enable(params)

	self.isEnabled = 1;
	self:SpawnInitially();
	BroadcastEvent(self, "Enable");
end

--------------------------------------------------------------------------
function AISpawner:Event_Disable(params)

	self.isEnabled = 0;
	BroadcastEvent(self, "Disable");
end

--------------------------------------------------------------------------
function AISpawner:Event_Limit(params)

	if(self.Properties.bLimitStop ~= 0) then
		self:Event_Disable();
	end
	BroadcastEvent(self, "Limit");
end

--------------------------------------------------------------------------
function AISpawner:Event_AutoDisableOn(params)

	for idx, spawnedId in pairs(self.spawnedIds) do
		local spawnedEntity = System.GetEntity( spawnedId );
		if( spawnedEntity ) then
			AI.AutoDisable( spawnedEntity.id, 1 );
			if(spawnedEntity.AutoDisablePassangers) then
				spawnedEntity:AutoDisablePassangers( 1 );
			end	
		end
	end
	BroadcastEvent(self, "AutoDisableOn");
end

--------------------------------------------------------------------------
function AISpawner:Event_AutoDisableOff(params)

	for idx, spawnedId in pairs(self.spawnedIds) do
		local spawnedEntity = System.GetEntity( spawnedId );
		if( spawnedEntity ) then
			AI.AutoDisable( spawnedEntity.id, 0 );
			if(spawnedEntity.AutoDisablePassangers) then
				spawnedEntity:AutoDisablePassangers( 0 );
			end	
		end
	end
	BroadcastEvent(self, "AutoDisableOff");
end


--------------------------------------------------------------------------
AISpawner.FlowEvents =
{
	Inputs =
	{
		Enable = { AISpawner.Event_Enable, "bool" },		
		Disable = { AISpawner.Event_Disable, "bool" },
		
		AutoDisableOn = { AISpawner.Event_AutoDisableOn, "bool" },
		AutoDisableOff = { AISpawner.Event_AutoDisableOff, "bool" },
		
--		Spawn = { AISpawner.Event_Spawn, "bool" },		
	},
	Outputs =
	{
		Limit = "bool",
	},
}
