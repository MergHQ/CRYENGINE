-- AUTOMATICALLY GENERATED CODE
-- use sandbox (AI/Generate Spawner Scripts) to regenerate this file
Script.ReloadScript("Scripts/Entities/AI/Civilian.lua")
SpawnCivilian =
{
	spawnedEntity = nil,
	Properties =
	{
		fileModel = "",
		attackrange = 0,
		commrange = 30,
		special = 0,
		aicharacter_character = "Hostage",
		Perception =
		{
			stanceScale = 1.8,
			sightrange = 50,
			FOVSecondary = 160,
			FOVPrimary = 80,
			audioScale = 1,
		},
		species = 10,
		bInvulnerable = 0,
		bSpeciesHostility = 0,
		accuracy = 1.0,
		soclasses_SmartObjectClass = "Actor",
	},
	PropertiesInstance =
	{
		aibehavior_behaviour = "Job_StandIdle",
		soclasses_SmartObjectClass = "",
		groupid = 173,
	},
}
SpawnCivilian.Properties.SpawnedEntityName = ""
function SpawnCivilian:Event_Spawn(sender,params)
	local params = {
		class = "Civilian",
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
function SpawnCivilian:OnReset()
	if self.spawnedEntity then
		System.RemoveEntity(self.spawnedEntity)
		self.spawnedEntity = nil
	end
end
function SpawnCivilian:GetFlowgraphForwardingEntity()
	return self.spawnedEntity
end
function SpawnCivilian:Event_Spawned()
	BroadcastEvent(self, "Spawned")
end
function SpawnCivilian:Event_Dead(sender, param)
	if sender and sender.id == self.spawnedEntity then BroadcastEvent(self, "Dead") end
end
function SpawnCivilian:Event_Disable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Disable(ent, sender, param)
		end
	end
end
function SpawnCivilian:Event_Enable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Enable(ent, sender, param)
		end
	end
end
function SpawnCivilian:Event_Kill(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Kill(ent, sender, param)
		end
	end
end
SpawnCivilian.FlowEvents =
{
	Inputs = 
	{
		Spawn = { SpawnCivilian.Event_Spawn, "bool" },
		Disable = { SpawnCivilian.Event_Disable, "bool" },
		Enable = { SpawnCivilian.Event_Enable, "bool" },
		Kill = { SpawnCivilian.Event_Kill, "bool" },
	},
	Outputs = 
	{
		Spawned = "bool",
		Dead = "bool",
	}
}
SpawnCivilian.Handle_Disable = Civilian.FlowEvents.Inputs.Disable[1]
SpawnCivilian.Handle_Enable = Civilian.FlowEvents.Inputs.Enable[1]
SpawnCivilian.Handle_Kill = Civilian.FlowEvents.Inputs.Kill[1]
