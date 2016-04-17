-- AUTOMATICALLY GENERATED CODE
-- use sandbox (AI/Generate Spawner Scripts) to regenerate this file
SpawnCoordinator =
{
	spawnedEntity = nil,
	Properties =
	{
		commrange = 30,
		bSpeciesHostility = 1,
		soclasses_SmartObjectClass = "Actor",
		attackrange = 70,
		special = 0,
		damageScale = 1,
		aicharacter_character = "Coordinator",
		Perception =
		{
			stanceScale = 1.8,
			sightrange = 50,
			FOVSecondary = 160,
			FOVPrimary = 80,
			audioScale = 1,
		},
		species = 2,
		bInvulnerable = 0,
		Vulnerability =
		{
			dmgMult = 1,
			dmgRatio = 1,
		},
		accuracy = 1.0,
		fileModel = "",
		horizontal_fov = 160,
	},
	PropertiesInstance =
	{
		aibehavior_behaviour = "CoordinatorIdle",
		soclasses_SmartObjectClass = "",
		groupid = 173,
	},
}
SpawnCoordinator.Properties.SpawnedEntityName = ""
function SpawnCoordinator:Event_Spawn(sender,params)
	local params = {
		class = "Coordinator",
		position = self:GetPos(),
		orientation = self:GetDirectionVector(1),
		scale = self:GetScale(),
		properties = self.Properties,
		propertiesInstance = self.PropertiesInstance,
	}
	if self.Properties.SpawnedEntityName ~= "" then
		params.name = self.Properties.SpawnedEntityName
	else
		params.name = self:GetName()
	end
	local ent = System.SpawnEntity(params)
	if ent then
		self.spawnedEntity = ent.id
		if not ent.Events then ent.Events = {} end
		local evts = ent.Events
		if not evts.Dead then evts.Dead = {} end
		table.insert(evts.Dead, {self.id, "Dead"})
	end
	BroadcastEvent(self, "Spawned")
end
function SpawnCoordinator:OnReset()
	if self.spawnedEntity then
		System.RemoveEntity(self.spawnedEntity)
		self.spawnedEntity = nil
	end
end
function SpawnCoordinator:GetFlowgraphForwardingEntity()
	return self.spawnedEntity
end
function SpawnCoordinator:Event_Spawned()
	BroadcastEvent(self, "Spawned")
end
function SpawnCoordinator:Event_Dead(sender, param)
	if sender and sender.id == self.spawnedEntity then BroadcastEvent(self, "Dead") end
end
function SpawnCoordinator:Event_Disable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Disable(ent, sender, param)
		end
	end
end
function SpawnCoordinator:Event_Enable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Enable(ent, sender, param)
		end
	end
end
function SpawnCoordinator:Event_Kill(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Kill(ent, sender, param)
		end
	end
end
SpawnCoordinator.FlowEvents =
{
	Inputs = 
	{
		Spawn = { SpawnCoordinator.Event_Spawn, "bool" },
		Disable = { SpawnCoordinator.Event_Disable, "bool" },
		Enable = { SpawnCoordinator.Event_Enable, "bool" },
		Kill = { SpawnCoordinator.Event_Kill, "bool" },
	},
	Outputs = 
	{
		Spawned = "bool",
		Dead = "bool",
	}
}
SpawnCoordinator.Handle_Disable = Coordinator.FlowEvents.Inputs.Disable[1]
SpawnCoordinator.Handle_Enable = Coordinator.FlowEvents.Inputs.Enable[1]
SpawnCoordinator.Handle_Kill = Coordinator.FlowEvents.Inputs.Kill[1]
